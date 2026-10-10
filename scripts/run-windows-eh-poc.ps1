# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.

param(
    [string]$InoxExe
)

$ErrorActionPreference = "Stop"
$PSNativeCommandUseErrorActionPreference = $false

if (-not $IsWindows) {
    throw "The MSVC EH PoC runs only on Windows."
}

$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($InoxExe)) {
    $InoxExe = Join-Path $repoRoot "build\windows-clang-msvc\Debug\inox.exe"
} elseif (-not [System.IO.Path]::IsPathRooted($InoxExe)) {
    $InoxExe = Join-Path $repoRoot $InoxExe
}

if (-not (Test-Path -LiteralPath $InoxExe -PathType Leaf)) {
    throw "Inox executable not found: $InoxExe"
}

$runtimeLib = Join-Path $repoRoot "build\windows-clang-msvc\Debug\inoxrt.lib"
if (-not (Test-Path -LiteralPath $runtimeLib -PathType Leaf)) {
    throw "Inox runtime library not found: $runtimeLib"
}

$clangxx = (Get-Command clang++ -ErrorAction Stop).Source

function Normalize-Output {
    param([string]$Text)
    return (($Text -replace "`r`n", "`n") -replace "`n+$", "")
}

function Invoke-ExpectedInoxOutput {
    param(
        [string]$TestPath,
        [string]$ExpectedPath,
        [bool]$ExpectSuccess
    )

    $actual = & $InoxExe "--run" $TestPath 2>&1 | Out-String
    $exitCode = $LASTEXITCODE
    $actual = Normalize-Output $actual
    $expected = Normalize-Output (Get-Content -LiteralPath $ExpectedPath -Raw)

    $exitOk = if ($ExpectSuccess) { $exitCode -eq 0 } else { $exitCode -ne 0 }
    if (-not $exitOk -or $actual -cne $expected) {
        throw "Inox PoC case failed: $TestPath`nexit=$exitCode`nexpected:`n$expected`nactual:`n$actual"
    }
}

function Invoke-CheckedNative {
    param(
        [string]$Exe,
        [string[]]$Arguments
    )

    $output = & $Exe @Arguments 2>&1 | Out-String
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        throw "Native command failed ($exitCode): $Exe $($Arguments -join ' ')`n$output"
    }
    return $output
}

# Criterion (i): real Inox plain except crosses the MSVC funclet bridge.
$plain = Join-Path $repoRoot "tests\integration\exceptions\plain-except.inox"
$plainOut = Join-Path $repoRoot "tests\integration\exceptions\plain-except.out"
Invoke-ExpectedInoxOutput -TestPath $plain -ExpectedPath $plainOut -ExpectSuccess $true
Write-Host "[PASS] (i) plain-except through MSVC catchpad bridge"

$tempDir = Join-Path ([System.IO.Path]::GetTempPath()) ("inox-msvc-eh-poc-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tempDir | Out-Null
try {
    $hostCpp = Join-Path $repoRoot "tests\native\windows-eh-poc-host.cpp"
    $bridgeLl = Join-Path $repoRoot "tests\native\windows-eh-poc-bridge.ll"
    $hostObj = Join-Path $tempDir "host.obj"
    $bridgeObj = Join-Path $tempDir "bridge.obj"
    $probeExe = Join-Path $tempDir "windows-eh-poc.exe"

    Invoke-CheckedNative $clangxx @("-std=c++20", "-c", $hostCpp, "-o", $hostObj) | Out-Null
    Invoke-CheckedNative $clangxx @("-c", $bridgeLl, "-o", $bridgeObj) | Out-Null
    Invoke-CheckedNative $clangxx @($hostObj, $bridgeObj, $runtimeLib, "-o", $probeExe) | Out-Null

    # Criterion (iii-a): foreign C++ exception reaches typeId 0 and Else catches it.
    $foreignElse = & $probeExe "foreign-else" 2>&1 | Out-String
    $foreignElseCode = $LASTEXITCODE
    if ($foreignElseCode -ne 0 -or
        (Normalize-Output $foreignElse) -cne "foreign-else-caught") {
        throw "foreign Else probe failed: exit=$foreignElseCode output=$foreignElse"
    }

    # Criterion (iii-b): without Else the same std::runtime_error object escapes intact.
    $foreignRethrow = & $probeExe "foreign-rethrow" 2>&1 | Out-String
    $foreignRethrowCode = $LASTEXITCODE
    if ($foreignRethrowCode -ne 0 -or
        (Normalize-Output $foreignRethrow) -cne "foreign-preserved") {
        throw "foreign rethrow probe failed: exit=$foreignRethrowCode output=$foreignRethrow"
    }
    Write-Host "[PASS] (iii) foreign C++ exception: Else catches; otherwise object is preserved"

    # Criterion (v): an Inox exception keeps its type id across the bridge.
    $inoxTyped = & $probeExe "inox-typed" 2>&1 | Out-String
    $inoxTypedCode = $LASTEXITCODE
    if ($inoxTypedCode -ne 0 -or
        (Normalize-Output $inoxTyped) -cne "inox-type-42") {
        throw "Inox typed probe failed: exit=$inoxTypedCode output=$inoxTyped"
    }
    Write-Host "[PASS] (v) Inox exception classified with its type id after catchret"

    # Criterion (ii): synchronous C++ catch-all must NOT intercept arbitrary SEH.
    # Success here is process termination by RaiseException. The bridge marker
    # and the post-call marker must both remain absent.
    $sehOutput = & $probeExe "seh" 2>&1 | Out-String
    $sehCode = $LASTEXITCODE
    if ($sehCode -eq 0 -or
        $sehOutput.Contains("seh-bridge-caught") -or
        $sehOutput.Contains("seh-returned")) {
        throw "SEH probe was intercepted or resumed: exit=$sehCode output=$sehOutput"
    }
    Write-Host "[PASS] (ii) arbitrary SEH bypasses the Inox C++ catch-all bridge"
}
finally {
    Remove-Item -LiteralPath $tempDir -Recurse -Force -ErrorAction SilentlyContinue
}

# Criterion (iv): P-A arithmetic faults remain non-EH traps. The exact .out
# proves that neither an exception handler nor the Ensure body executed; the
# driver line is emitted only after observing child exit status 70.
$fault = Join-Path $repoRoot "tests\runtime\fault-not-catchable.inox"
$faultOut = Join-Path $repoRoot "tests\runtime\fault-not-catchable.out"
Invoke-ExpectedInoxOutput -TestPath $fault -ExpectedPath $faultOut -ExpectSuccess $false
Write-Host "[PASS] (iv) arithmetic trap remains status 70 and bypasses handler/Ensure"

Write-Host "MSVC EH PoC: all four acceptance criteria passed."
