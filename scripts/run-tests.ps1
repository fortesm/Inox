# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

param(
    [string]$InoxExe
)

$ErrorActionPreference = "Stop"

# Programs print UTF-8 (CANON-14); decode their output as UTF-8, not with the
# console code page.
$utf8 = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = $utf8
$OutputEncoding = $utf8

# Runs `inox --run Test` with the bytes of InputPath as standard input, exactly
# as they are in the file: piping Get-Content would re-encode the text (an
# invalid UTF-8 byte would arrive as a valid U+FFFD) and rewrite line breaks.
function Invoke-InoxWithInput {
    param(
        [string]$TestPath,
        [string]$InputPath
    )
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new($InoxExe)
    $startInfo.ArgumentList.Add("--run")
    $startInfo.ArgumentList.Add($TestPath)
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.StandardOutputEncoding = $utf8
    $startInfo.StandardErrorEncoding = $utf8
    $process = [System.Diagnostics.Process]::Start($startInfo)
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $bytes = [System.IO.File]::ReadAllBytes($InputPath)
    try {
        $process.StandardInput.BaseStream.Write($bytes, 0, $bytes.Length)
        $process.StandardInput.BaseStream.Flush()
    } catch [System.IO.IOException] {
        # The program may stop (a trap) before it reads all of its input.
    }
    $process.StandardInput.Close()
    $process.WaitForExit()
    return [pscustomobject]@{
        Stdout = $stdoutTask.Result
        Stderr = $stderrTask.Result
        ExitCode = $process.ExitCode
    }
}

$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($InoxExe)) {
    $candidateExecutables = @(
        (Join-Path $repoRoot "build\windows-clang-msvc\Debug\inox.exe"),
        (Join-Path $repoRoot "build\Debug\inox.exe"),
        (Join-Path $repoRoot "build\linux-clang-debug\inox"),
        (Join-Path $repoRoot "build\inox"),
        (Join-Path $repoRoot "build-linux\inox"),
        (Join-Path $repoRoot "build-clang\inox")
    )

    $InoxExe = $candidateExecutables | Where-Object {
        Test-Path -LiteralPath $_ -PathType Leaf
    } | Select-Object -First 1

    if ([string]::IsNullOrWhiteSpace($InoxExe)) {
        $InoxExe = Join-Path $repoRoot "build\windows-clang-msvc\Debug\inox.exe"
    }
} elseif (-not [System.IO.Path]::IsPathRooted($InoxExe)) {
    $InoxExe = Join-Path $repoRoot $InoxExe
}

if (-not (Test-Path -LiteralPath $InoxExe -PathType Leaf)) {
    Write-Host "Inox executable not found: $InoxExe"
    Write-Host "Run one of:"
    Write-Host "  Windows: cmake --preset windows-clang-msvc; cmake --build --preset windows-clang-msvc-debug"
    Write-Host "  Linux:   cmake --preset linux-clang-debug; cmake --build --preset linux-clang-debug"
    exit 1
}

$passed = 0
$failed = 0

function Invoke-InoxTest {
    param(
        [System.IO.FileInfo]$TestFile,
        [bool]$ExpectSuccess
    )

    $relativePath = [System.IO.Path]::GetRelativePath($repoRoot, $TestFile.FullName)
    & $InoxExe "check" $TestFile.FullName *> $null
    $exitCode = $LASTEXITCODE

    if ($ExpectSuccess) {
        $ok = $exitCode -eq 0
        $expectation = "success"
    } else {
        $ok = $exitCode -ne 0
        $expectation = "failure"
    }

    if ($ok) {
        $script:passed++
        Write-Host "[PASS] $relativePath"
    } else {
        $script:failed++
        Write-Host "[FAIL] $relativePath (expected $expectation, exit code $exitCode)"
    }
}

function Invoke-LlvmEmissionTest {
    param(
        [System.IO.FileInfo]$TestFile,
        [string[]]$RequiredFragments
    )

    $relativePath = [System.IO.Path]::GetRelativePath($repoRoot, $TestFile.FullName)
    $output = & $InoxExe "--emit-llvm" $TestFile.FullName 2>&1 | Out-String
    $exitCode = $LASTEXITCODE
    $missingFragments = @($RequiredFragments | Where-Object { -not $output.Contains($_) })
    $ok = $exitCode -eq 0 -and $missingFragments.Count -eq 0

    if ($ok) {
        $script:passed++
        Write-Host "[PASS] $relativePath --emit-llvm"
    } else {
        $script:failed++
        Write-Host "[FAIL] $relativePath --emit-llvm"
        Write-Host "       expected exit code 0 and all required LLVM fragments"
        Write-Host "       actual exit code: $exitCode"
        if ($missingFragments.Count -ne 0) {
            Write-Host "       missing: $($missingFragments -join ', ')"
        }
    }
}


function Invoke-ModeExitTest {
    param(
        [string]$Mode,
        [System.IO.FileInfo]$TestFile,
        [bool]$ExpectSuccess
    )

    $relativePath = [System.IO.Path]::GetRelativePath($repoRoot, $TestFile.FullName)
    & $InoxExe $Mode $TestFile.FullName *> $null
    $exitCode = $LASTEXITCODE

    if ($ExpectSuccess) {
        $ok = $exitCode -eq 0
        $expectation = "success"
    } else {
        $ok = $exitCode -ne 0
        $expectation = "failure"
    }

    if ($ok) {
        $script:passed++
        Write-Host "[PASS] $relativePath $Mode"
    } else {
        $script:failed++
        Write-Host "[FAIL] $relativePath $Mode (expected $expectation, exit code $exitCode)"
    }
}

function Invoke-ModeFragmentTest {
    param(
        [string]$Mode,
        [System.IO.FileInfo]$TestFile,
        [string[]]$RequiredFragments
    )

    $relativePath = [System.IO.Path]::GetRelativePath($repoRoot, $TestFile.FullName)
    $output = & $InoxExe $Mode $TestFile.FullName 2>&1 | Out-String
    $exitCode = $LASTEXITCODE
    $missingFragments = @($RequiredFragments | Where-Object { -not $output.Contains($_) })
    $ok = $exitCode -eq 0 -and $missingFragments.Count -eq 0

    if ($ok) {
        $script:passed++
        Write-Host "[PASS] $relativePath $Mode"
    } else {
        $script:failed++
        Write-Host "[FAIL] $relativePath $Mode"
        Write-Host "       expected exit code 0 and all required fragments"
        Write-Host "       actual exit code: $exitCode"
        if ($missingFragments.Count -ne 0) {
            Write-Host "       missing: $($missingFragments -join ', ')"
        }
    }
}

function Invoke-LinkedExecutionTest {
    param(
        [System.IO.FileInfo]$TestFile,
        [System.IO.FileInfo]$ExpectedOutputFile
    )

    $relativePath = [System.IO.Path]::GetRelativePath($repoRoot, $TestFile.FullName)
    # E1 (change console-abi-terminal): every program links libinoxrt.
    $clang = Get-Command clang++ -ErrorAction SilentlyContinue
    if ($null -eq $clang) {
        Write-Host "[SKIP] $relativePath link/run (clang++ not found)"
        return
    }
    $runtimeLib = $env:INOX_RUNTIME_LIB
    if ([string]::IsNullOrEmpty($runtimeLib)) {
        $inoxDir = Split-Path -Parent $InoxExe
        $runtimeLib = Join-Path $inoxDir "inoxrt.lib"
        if (-not (Test-Path -LiteralPath $runtimeLib)) {
            $runtimeLib = Join-Path $inoxDir "libinoxrt.a"
        }
    }

    $tempDir = Join-Path ([System.IO.Path]::GetTempPath()) ("inox-test-" + [System.Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $tempDir | Out-Null
    try {
        $llPath = Join-Path $tempDir "program.ll"
        $exePath = Join-Path $tempDir "program.exe"
        & $InoxExe "--emit-llvm" $TestFile.FullName > $llPath
        $emitExit = $LASTEXITCODE
        if ($emitExit -ne 0) {
            $script:failed++
            Write-Host "[FAIL] $relativePath link/run"
            Write-Host "       LLVM emission failed with exit code $emitExit"
            return
        }

        $clangArgs = @($llPath, $runtimeLib, "-o", $exePath)
        if (-not $IsWindows) {
            $clangArgs += "-lm"
        }
        & $clang.Source @clangArgs *> $null
        $clangExit = $LASTEXITCODE
        if ($clangExit -ne 0) {
            $script:failed++
            Write-Host "[FAIL] $relativePath link/run"
            Write-Host "       clang link failed with exit code $clangExit"
            return
        }

        $actual = & $exePath 2>&1 | Out-String
        $expected = Get-Content -LiteralPath $ExpectedOutputFile.FullName -Raw
        $actual = ($actual -replace "`r`n", "`n") -replace "`n+$", ""
        $expected = ($expected -replace "`r`n", "`n") -replace "`n+$", ""

        if ($actual -eq $expected) {
            $script:passed++
            Write-Host "[PASS] $relativePath link/run"
        } else {
            $script:failed++
            Write-Host "[FAIL] $relativePath link/run"
            Write-Host "       expected output:"
            Write-Host $expected
            Write-Host "       actual output:"
            Write-Host $actual
        }
    } finally {
        Remove-Item -LiteralPath $tempDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

function Invoke-BuildDriverTest {
    param(
        [System.IO.FileInfo]$TestFile
    )

    $relativePath = [System.IO.Path]::GetRelativePath($repoRoot, $TestFile.FullName)
    $clang = Get-Command clang -ErrorAction SilentlyContinue
    if ($null -eq $clang) {
        Write-Host "[SKIP] $relativePath --build (clang not found)"
        return
    }

    & $InoxExe "--build" $TestFile.FullName *> $null
    $exitCode = $LASTEXITCODE
    if ($exitCode -eq 0) {
        $script:passed++
        Write-Host "[PASS] $relativePath --build"
    } else {
        $script:failed++
        Write-Host "[FAIL] $relativePath --build (exit code $exitCode)"
    }
}

# The driver must show the toolchain's own diagnostics when clang fails. The
# failure is provoked portably: a directory occupies the executable path, so the
# link step cannot write its output.
function Invoke-ToolchainFailureTest {
    param(
        [System.IO.FileInfo]$TestFile
    )

    $relativePath = [System.IO.Path]::GetRelativePath($repoRoot, $TestFile.FullName)
    $clang = Get-Command clang -ErrorAction SilentlyContinue
    if ($null -eq $clang) {
        Write-Host "[SKIP] $relativePath --build (toolchain diagnostics; clang not found)"
        return
    }

    $stem = [System.IO.Path]::GetFileNameWithoutExtension($TestFile.Name)
    $outDir = Join-Path ([System.IO.Path]::GetTempPath()) ("inox-toolchain-" + [System.Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path (Join-Path $outDir $stem) -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $outDir ($stem + ".exe")) -Force | Out-Null
    $previousOutputDir = $env:INOX_OUTPUT_DIR
    $env:INOX_OUTPUT_DIR = $outDir
    try {
        $actual = (& $InoxExe "--build" $TestFile.FullName 2>&1 | Out-String)
        $exitCode = $LASTEXITCODE
    } finally {
        $env:INOX_OUTPUT_DIR = $previousOutputDir
        Remove-Item -LiteralPath $outDir -Recurse -Force -ErrorAction SilentlyContinue
    }
    if ($exitCode -ne 0 -and $actual.Contains("failed while building") -and
        $actual.Contains("linker command failed") -and $actual.Contains("full toolchain output")) {
        $script:passed++
        Write-Host "[PASS] $relativePath --build (toolchain diagnostics)"
    } else {
        $script:failed++
        Write-Host "[FAIL] $relativePath --build (toolchain diagnostics)"
        Write-Host "       exit code: $exitCode"
        Write-Host "       actual error: $actual"
    }
}

function Invoke-RunDriverTest {
    # Command (CANON-23): "--run" (default), "run", or "-" for `inox <file>`
    # alone, which also builds and runs.
    param(
        [System.IO.FileInfo]$TestFile,
        [System.IO.FileInfo]$ExpectedOutputFile,
        [string]$Command = "--run"
    )

    $relativePath = [System.IO.Path]::GetRelativePath($repoRoot, $TestFile.FullName)
    $label = if ($Command -eq "-") { "$relativePath (inox <file>)" } else { "$relativePath $Command" }
    $clang = Get-Command clang -ErrorAction SilentlyContinue
    if ($null -eq $clang) {
        Write-Host "[SKIP] $label (clang not found)"
        return
    }

    if ($Command -eq "-") {
        $actual = & $InoxExe $TestFile.FullName 2>&1 | Out-String
    } else {
        $actual = & $InoxExe $Command $TestFile.FullName 2>&1 | Out-String
    }
    $exitCode = $LASTEXITCODE
    $expected = Get-Content -LiteralPath $ExpectedOutputFile.FullName -Raw
    $actual = ($actual -replace "`r`n", "`n") -replace "`n+$", ""
    $expected = ($expected -replace "`r`n", "`n") -replace "`n+$", ""

    if ($exitCode -eq 0 -and $actual -eq $expected) {
        $script:passed++
        Write-Host "[PASS] $label"
    } else {
        $script:failed++
        Write-Host "[FAIL] $label"
        Write-Host "       exit code: $exitCode"
        Write-Host "       expected output:"
        Write-Host $expected
        Write-Host "       actual output:"
        Write-Host $actual
    }
}

function Invoke-RunDriverInputTest {
    param(
        [System.IO.FileInfo]$TestFile,
        [System.IO.FileInfo]$InputFile,
        [System.IO.FileInfo]$ExpectedOutputFile
    )

    $relativePath = [System.IO.Path]::GetRelativePath($repoRoot, $TestFile.FullName)
    $clang = Get-Command clang -ErrorAction SilentlyContinue
    if ($null -eq $clang) {
        Write-Host "[SKIP] $relativePath --run < input (clang not found)"
        return
    }

    $result = Invoke-InoxWithInput -TestPath $TestFile.FullName -InputPath $InputFile.FullName
    $actual = $result.Stdout + $result.Stderr
    $exitCode = $result.ExitCode
    $expected = Get-Content -LiteralPath $ExpectedOutputFile.FullName -Raw
    $actual = ($actual -replace "`r`n", "`n") -replace "`n+$", ""
    $expected = ($expected -replace "`r`n", "`n") -replace "`n+$", ""

    if ($exitCode -eq 0 -and $actual -eq $expected) {
        $script:passed++
        Write-Host "[PASS] $relativePath --run < input"
    } else {
        $script:failed++
        Write-Host "[FAIL] $relativePath --run < input"
        Write-Host "       exit code: $exitCode"
        Write-Host "       expected output:"
        Write-Host $expected
        Write-Host "       actual output:"
        Write-Host $actual
    }
}

function Invoke-DriverTrapTest {
    param(
        [System.IO.FileInfo]$TestFile,
        [string]$InputPath = ""
    )

    $relativePath = [System.IO.Path]::GetRelativePath($repoRoot, $TestFile.FullName)
    $clang = Get-Command clang -ErrorAction SilentlyContinue
    if ($null -eq $clang) {
        Write-Host "[SKIP] $relativePath --run (expect trap; clang not found)"
        return
    }

    # NAME.trap holds the diagnostic the program must print, for example
    # "Inox runtime error: division by zero" (CANON-19).
    $trapPath = [System.IO.Path]::ChangeExtension($TestFile.FullName, ".trap")
    $expectedMessage = ((Get-Content -LiteralPath $trapPath -TotalCount 1) -replace "`r$", "").Trim()

    # Standard output and standard error are captured separately: when both
    # are piped, Windows gives no ordering between the two streams, so a merged
    # capture can show the runtime diagnostic before output the program had
    # already flushed.
    $stderrPath = [System.IO.Path]::GetTempFileName()
    try {
        if ($InputPath -ne "") {
            $result = Invoke-InoxWithInput -TestPath $TestFile.FullName -InputPath $InputPath
            $stdout = $result.Stdout
            $exitCode = $result.ExitCode
            $stderr = $result.Stderr
        } else {
            $stdout = & $InoxExe "--run" $TestFile.FullName 2>$stderrPath | Out-String
            $exitCode = $LASTEXITCODE
            $stderr = Get-Content -LiteralPath $stderrPath -Raw
        }
    } finally {
        Remove-Item -LiteralPath $stderrPath -Force -ErrorAction SilentlyContinue
    }
    if ($null -eq $stderr) { $stderr = "" }
    $stdout = ($stdout -replace "`r`n", "`n") -replace "`n+$", ""
    $stderr = ($stderr -replace "`r`n", "`n") -replace "`n+$", ""
    $actual = (@($stdout, $stderr) | Where-Object { $_ -ne "" }) -join "`n"

    # A compile error also exits non-zero, so require the runtime diagnostic itself.
    # Optional NAME.out next to NAME.trap: the complete output (program output,
    # runtime diagnostic, driver note) must match exactly, which proves that no
    # handler or ensure block ran.
    $exactPath = [System.IO.Path]::ChangeExtension($TestFile.FullName, ".out")
    $exactOk = $true
    if (Test-Path -LiteralPath $exactPath) {
        # The runtime diagnostic and the driver note go to standard error; the
        # rest of NAME.out is the program's standard output, in order.
        $exactLines = (((Get-Content -LiteralPath $exactPath -Raw) -replace "`r`n", "`n") -replace "`n+$", "") -split "`n"
        $isDiagnostic = { param($line) $line.StartsWith("Inox runtime error:") -or $line.StartsWith("inox:") }
        $expectedOut = ($exactLines | Where-Object { -not (& $isDiagnostic $_) }) -join "`n"
        $expectedErr = ($exactLines | Where-Object { & $isDiagnostic $_ }) -join "`n"
        $exactOk = ($stdout -ceq $expectedOut) -and ($stderr -ceq $expectedErr)
    }
    if ($exitCode -ne 0 -and $expectedMessage -ne "" -and $actual.Contains($expectedMessage) -and $exactOk) {
        $script:passed++
        Write-Host "[PASS] $relativePath --run (trap)"
    } else {
        $script:failed++
        Write-Host "[FAIL] $relativePath --run (trap)"
        Write-Host "       expected the program to compile, run and stop with: $expectedMessage"
        Write-Host "       exit code: $exitCode"
        Write-Host "       actual output:"
        Write-Host $actual
    }
}

# Runs every NAME.inox in a directory according to its sidecar files:
#   NAME.out  exit 0 and exactly this output     NAME.trap  must trap at run time
#   NAME.in   optional standard input
function Invoke-RuntimeTree {
    param([string]$RelativePath)

    $root = Join-Path $repoRoot $RelativePath
    if (-not (Test-Path -LiteralPath $root)) {
        return
    }

    Get-ChildItem -LiteralPath $root -Filter "*.inox" -File | Sort-Object Name | ForEach-Object {
        $testFile = $_
        $base = Join-Path $testFile.DirectoryName ([System.IO.Path]::GetFileNameWithoutExtension($testFile.Name))
        $inputPath = "$base.in"
        $hasInput = Test-Path -LiteralPath $inputPath
        if (Test-Path -LiteralPath "$base.trap") {
            if ($hasInput) {
                Invoke-DriverTrapTest -TestFile $testFile -InputPath $inputPath
            } else {
                Invoke-DriverTrapTest -TestFile $testFile
            }
        } elseif (Test-Path -LiteralPath "$base.out") {
            if ($hasInput) {
                Invoke-RunDriverInputTest `
                    -TestFile $testFile `
                    -InputFile (Get-Item -LiteralPath $inputPath) `
                    -ExpectedOutputFile (Get-Item -LiteralPath "$base.out")
            } else {
                Invoke-RunDriverTest `
                    -TestFile $testFile `
                    -ExpectedOutputFile (Get-Item -LiteralPath "$base.out")
            }
        }
    }
}

# Each NAME.inox must be rejected, and the error output must contain NAME.err.
# Every examples\NAME.inox must build and run: its expected output is
# tests\examples\NAME.out (and its input, if any, tests\examples\NAME.in), so
# the examples directory holds only programs.
function Invoke-Examples {
    $expectedRoot = Join-Path $repoRoot "tests\examples"
    Get-ChildItem -LiteralPath (Join-Path $repoRoot "examples") -Filter "*.inox" -File | Sort-Object Name | ForEach-Object {
        $example = $_
        $name = [System.IO.Path]::GetFileNameWithoutExtension($example.Name)
        $expectedPath = Join-Path $expectedRoot "$name.out"
        $inputPath = Join-Path $expectedRoot "$name.in"
        if (-not (Test-Path -LiteralPath $expectedPath)) {
            $script:failed++
            Write-Host "[FAIL] examples\$($example.Name) runs"
            Write-Host "       missing expected output: $expectedPath"
        } elseif (Test-Path -LiteralPath $inputPath) {
            Invoke-RunDriverInputTest `
                -TestFile $example `
                -InputFile (Get-Item -LiteralPath $inputPath) `
                -ExpectedOutputFile (Get-Item -LiteralPath $expectedPath)
        } else {
            Invoke-RunDriverTest `
                -TestFile $example `
                -ExpectedOutputFile (Get-Item -LiteralPath $expectedPath)
        }
    }
}

function Invoke-DiagnosticTree {
    param([string]$RelativePath)

    $root = Join-Path $repoRoot $RelativePath
    if (-not (Test-Path -LiteralPath $root)) {
        return
    }

    Get-ChildItem -LiteralPath $root -Filter "*.inox" -File | Sort-Object Name | ForEach-Object {
        $testFile = $_
        $relativeFile = [System.IO.Path]::GetRelativePath($repoRoot, $testFile.FullName)
        $expectedPath = Join-Path $testFile.DirectoryName ([System.IO.Path]::GetFileNameWithoutExtension($testFile.Name) + ".err")
        if (-not (Test-Path -LiteralPath $expectedPath)) {
            $script:failed++
            Write-Host "[FAIL] $relativeFile diagnostic"
            Write-Host "       missing expectation file: $expectedPath"
            return
        }
        $expected = ((Get-Content -LiteralPath $expectedPath -Raw) -replace "`r`n", "`n") -replace "`n+$", ""
        $actual = & $InoxExe "--emit-llvm" $testFile.FullName 2>&1 | Out-String
        $exitCode = $LASTEXITCODE
        if ($exitCode -ne 0 -and $actual.Contains($expected)) {
            $script:passed++
            Write-Host "[PASS] $relativeFile diagnostic"
        } else {
            $script:failed++
            Write-Host "[FAIL] $relativeFile diagnostic"
            Write-Host "       exit code: $exitCode"
            Write-Host "       expected error containing: $expected"
            Write-Host "       actual error: $actual"
        }
    }
}

function Get-InoxTestFiles {
    param(
        [string]$RelativePath,
        [switch]$Recurse
    )

    $root = Join-Path $repoRoot $RelativePath
    if (-not (Test-Path -LiteralPath $root)) {
        return @()
    }

    if ($Recurse) {
        return @(Get-ChildItem -LiteralPath $root -Filter "*.inox" -File -Recurse | Sort-Object FullName)
    }

    return @(Get-ChildItem -LiteralPath $root -Filter "*.inox" -File | Sort-Object FullName)
}

$validTestRoots = @(
    @{ Path = "examples"; Recurse = $false },
    @{ Path = "tests\parser\valid"; Recurse = $true },
    @{ Path = "tests\semantic\valid"; Recurse = $true }
)

$invalidTestRoots = @(
    @{ Path = "tests\invalid"; Recurse = $false },
    @{ Path = "tests\lexer\invalid"; Recurse = $true },
    @{ Path = "tests\parser\invalid"; Recurse = $true },
    @{ Path = "tests\semantic\invalid"; Recurse = $true }
)

foreach ($rootSpec in $validTestRoots) {
    foreach ($test in Get-InoxTestFiles -RelativePath $rootSpec.Path -Recurse:([bool]$rootSpec.Recurse)) {
        Invoke-InoxTest -TestFile $test -ExpectSuccess $true
    }
}

foreach ($rootSpec in $invalidTestRoots) {
    foreach ($test in Get-InoxTestFiles -RelativePath $rootSpec.Path -Recurse:([bool]$rootSpec.Recurse)) {
        Invoke-InoxTest -TestFile $test -ExpectSuccess $false
    }
}

Invoke-ModeFragmentTest `
    -Mode "--dump-tokens" `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\lexer\valid\tokens-keywords-literals.inox")) `
    -RequiredFragments @('Identifier lexeme="Main" normalized="main"', 'Keyword lexeme="Type" normalized="type"', 'Keyword lexeme="Struct" normalized="struct"', 'Keyword lexeme="Retry" normalized="retry"', 'IntegerLiteral lexeme="$2A"', 'StringLiteral lexeme="hello"', 'CharLiteral lexeme=', 'Identifier lexeme="End" normalized="end"', 'Identifier lexeme="Leave" normalized="leave"', 'Keyword lexeme="Ensure" normalized="ensure"', 'Identifier lexeme="Break" normalized="break"', 'Identifier lexeme="Finally" normalized="finally"')

Invoke-ModeExitTest `
    -Mode "--parse-only" `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\parser\valid\canonical-type-and-var.inox")) `
    -ExpectSuccess $true

Invoke-ModeExitTest `
    -Mode "--parse-only" `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\parser\invalid\var-colon.inox")) `
    -ExpectSuccess $false

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\empty.inox")) `
    -RequiredFragments @("define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\integer-function.inox")) `
    -RequiredFragments @("define i64 @inox_sum", "%tmp0 = call i64 @__inox_add_i64(i64 %a, i64 %b)", "ret i64 %tmp0", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\function-call.inox")) `
    -RequiredFragments @("define i64 @inox_sum", "define i64 @inox_double", "%tmp0 = call i64 @inox_sum(i64 %x, i64 %x)", "ret i64 %tmp0", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\local-variables.inox")) `
    -RequiredFragments @("define i64 @inox_compute", "%a = alloca i64", "%b = alloca i64", "store i64 10, ptr %a", "store i64 20, ptr %b", "load i64, ptr %a", "load i64, ptr %b", "call i64 @__inox_add_i64", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\inline-typed-local.inox")) `
    -RequiredFragments @("define i64 @inox_compute", "%a = alloca i64", "%b = alloca i64", "store i64 10, ptr %a", "store i64 20, ptr %b", "load i64, ptr %a", "load i64, ptr %b", "call i64 @__inox_add_i64", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\local-assignment.inox")) `
    -RequiredFragments @("define i64 @inox_compute", "%a = alloca i64", "%b = alloca i64", "store i64 10, ptr %a", "store i64 20, ptr %b", "call i64 @__inox_add_i64", "call i64 @__inox_mul_i64", "store i64 %tmp0, ptr %a", "store i64 %tmp3, ptr %b", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\integer-operators.inox")) `
    -RequiredFragments @("define i64 @inox_compute", "%tmp0 = call i64 @__inox_div_i64(i64 %a, i64 %b)", "call i64 @__inox_mod_i64", "call i64 @__inox_shl_i64", "call i64 @__inox_shr_i64", "and i64", "or i64", "xor i64", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\bool-comparisons.inox")) `
    -RequiredFragments @("define i1 @inox_isgreater", "define i1 @inox_isequal", "define i1 @inox_isdifferent", "icmp sgt i64", "icmp eq i64", "icmp ne i64", "icmp slt i64", "icmp sle i64", "icmp sge i64", "ret i1", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\bool-operators.inox")) `
    -RequiredFragments @("define i1 @inox_both", "define i1 @inox_either", "define i1 @inox_different", "define i1 @inox_notpositive", "and i1", "or i1", "xor i1", "xor i1 %tmp0, true", "ret i1", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\if-return.inox")) `
    -RequiredFragments @("define i64 @inox_max", "icmp sgt i64", "br i1", "label %then0", "label %else0", "then0:", "else0:", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\if-merge.inox")) `
    -RequiredFragments @("define i64 @inox_maxplusone", "%m = alloca i64", "icmp sgt i64", "br i1", "label %then0", "label %else0", "then0:", "else0:", "br label %endif0", "endif0:", "store i64", "load i64", "call i64 @__inox_add_i64", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\while-loop.inox")) `
    -RequiredFragments @("define i64 @inox_sumto", "whilecond0:", "whilebody0:", "whileend0:", "br i1", "br label %whilecond0", "icmp sgt i64", "call i64 @__inox_add_i64", "call i64 @__inox_sub_i64", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\while-leave-continue.inox")) `
    -RequiredFragments @("define i64 @inox_findfirstbelow", "whilecond0:", "whilebody0:", "whileend0:", "br i1", "br label %whilecond0", "br label %whileend0", "icmp eq i64", "call i64 @__inox_sub_i64", "store i64", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\if-no-else.inox")) `
    -RequiredFragments @("define i64 @inox_clamppositive", "%x = alloca i64", "icmp slt i64", "br i1", "label %then0", "label %endif0", "then0:", "br label %endif0", "endif0:", "store i64", "load i64", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\elif-return.inox")) `
    -RequiredFragments @("define i64 @inox_compare", "icmp sgt i64", "icmp eq i64", "br i1", "elifcond0_0:", "elifthen0_0:", "ret i64 1", "ret i64 0", "ret i64 -1", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\repeat-flexible-end.inox")) `
    -RequiredFragments @("define i64 @inox_countdown", "repeatbody", "repeatend", "br i1", "br label", "icmp", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\repeat-flexible-start.inox")) `
    -RequiredFragments @("define i64 @inox_countdown", "repeatbody", "repeatend", "br i1", "br label", "icmp", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\repeat-flexible-middle.inox")) `
    -RequiredFragments @("define i64 @inox_countdown", "repeatbody", "repeatend", "br i1", "br label", "icmp", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\repeat-leave-continue.inox")) `
    -RequiredFragments @("define i64 @inox_findvalue", "repeatbody", "repeatend", "br i1", "br label", "icmp eq i64", "call i64 @__inox_sub_i64", "store i64", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\for-range-leave-continue.inox")) `
    -RequiredFragments @("define i64 @inox_sumrange", "forcond", "forbody", "forstep", "forend", "br i1", "br label", "icmp sle i64", "icmp eq i64", "call i64 @__inox_add_i64", "@llvm.sadd.with.overflow.i64", "store i64", "load i64", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\for-range-step.inox")) `
    -RequiredFragments @("define i64 @inox_sumevenuntil", "forcond", "forbody", "forstep", "forend", "store i64 2, ptr %i", "icmp sle i64", "icmp eq i64", "call i64 @__inox_add_i64", "@__inox_for_step_i64(i64 2)", "@llvm.sadd.with.overflow.i64", "br i1", "br label", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\putln-integer.inox")) `
    -RequiredFragments @("declare void @__inox_con_put_i64(i64)", "define i64 @inox_value", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "call void @__inox_runtime_init(i32 %__inox.argc, ptr %__inox.argv)", "call void @__inox_con_put_i64", "call void @__inox_con_put_newline()", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\put-output-basic.inox")) `
    -RequiredFragments @("@.inox.str.", "call void @__inox_con_write(ptr @.inox.str.", "call void @__inox_con_put_bool(i1 zeroext", "call void @__inox_con_put_newline()", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\subroutine-calls.inox")) `
    -RequiredFragments @("define i64 @inox_value", "define void @inox_report", "call void @inox_report", "ret void", "report=", "call void @__inox_con_write", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\struct-basic.inox")) `
    -RequiredFragments @("%tpoint = type { i64, i64 }", "define i64 @inox_sumpoint", "alloca %tpoint", "zeroinitializer", "getelementptr %tpoint", "store i64 10", "store i64 20", "load i64", "call i64 @__inox_add_i64", "call i64 @inox_sumpoint", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\associated-methods.inox")) `
    -RequiredFragments @("%tpoint = type { i64, i64 }", "define void @inox_tpoint.move", "define i64 @inox_tpoint.sum", "ptr %self", "call void @inox_tpoint.move", "call i64 @inox_tpoint.sum", "getelementptr %tpoint", "ret void", "ret i64", "define i32 @main(i32 %__inox.argc, ptr %__inox.argv)", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\struct-field-defaults.inox")) `
    -RequiredFragments @("%tconfig = type { i64, i1 }", "define i64 @inox_getport", "alloca %tconfig", "zeroinitializer", "store i64 8080", "store i1 1", "getelementptr %tconfig", "load i64", "call i64 @inox_getport", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\with-statement.inox")) `
    -RequiredFragments @("%tpoint = type { i64, i64 }", "define i64 @inox_sumpoint", "alloca %tpoint", "getelementptr %tpoint", "store i64 10", "store i64 20", "load i64", "call i64 @__inox_add_i64", "call i64 @inox_sumpoint", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\struct-values.inox")) `
    -RequiredFragments @("%tpoint = type { i64, i64 }", "define %tpoint @inox_makepoint", "define i64 @inox_sumpoint", "define %tpoint @inox_copypoint", "%p.addr = alloca %tpoint", "store %tpoint %p, ptr %p.addr", "load %tpoint", "ret %tpoint", "call %tpoint @inox_makepoint", "call %tpoint @inox_copypoint", "call i64 @inox_sumpoint", "ret i32 0")


Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\codegen\llvm-struct-value-smoke.inox")) `
    -RequiredFragments @("%tpair = type { i64, i64 }", "define %tpair @inox_makepair", "define i64 @inox_sumpair", "call %tpair @inox_makepair", "call i64 @inox_sumpair", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\codegen\llvm-string-append-smoke.inox")) `
    -RequiredFragments @("call ptr @__inox_str_concat(i64 3", "call void @__inox_str_append(ptr %s, i64 1", "call void @__inox_str_append(ptr %s, i64 2", "call ptr @__inox_str_concat(i64 2", "ret i32 0")

$exceptionSmokeFragments = if ($IsWindows) {
    @("personality ptr @__CxxFrameHandler3", "catchswitch within none",
      "catchpad within", "[ptr null, i32 64, ptr null]",
      "call ptr @__inox_exception_capture(ptr null)",
      "call i64 @__inox_exception_type", "call void @__inox_exception_release",
      "call void @__inox_exception_rethrow")
} else {
    @("personality ptr @__gxx_personality_v0", "invoke void @inox_fail()",
      "landingpad { ptr, i32 } catch ptr null", "call i64 @__inox_exception_type",
      "call void @__inox_exception_release", "call void @__inox_exception_rethrow")
}
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\codegen\llvm-exceptions-smoke.inox")) `
    -RequiredFragments $exceptionSmokeFragments

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\codegen\llvm-exceptions-retry-smoke.inox")) `
    -RequiredFragments @("%eh.retry.slot", "%eh.action.slot", "icmp slt i64", "store i32 2", "switch i32", "eh.retry.perform")

Invoke-LinkedExecutionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\output-basic.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\output-basic.out"))

Invoke-BuildDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\run-hello.inox"))
Invoke-ToolchainFailureTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\run-hello.inox"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\run-hello.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\run-hello.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\run-hello.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\run-hello.out")) `
    -Command "run"
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\run-hello.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\run-hello.out")) `
    -Command "-"

# Driver commands (CANON-23): `check` checks without building, `help` and
# `version` are aliases of their switches, a command without a file is an error.
$helloPath = Join-Path $repoRoot "tests\integration\run-hello.inox"
$checkOutput = & $InoxExe "check" $helloPath 2>&1 | Out-String
if ($LASTEXITCODE -eq 0 -and $checkOutput -match "semantic ok") {
    $passed++; Write-Host "[PASS] inox check <file>"
} else {
    $failed++; Write-Host "[FAIL] inox check <file>"; Write-Host "       actual output: $checkOutput"
}
$versionWord = (& $InoxExe "version" 2>&1 | Out-String).Trim()
$versionSwitch = (& $InoxExe "--version" 2>&1 | Out-String).Trim()
if ($versionWord -eq $versionSwitch) {
    $passed++; Write-Host "[PASS] inox version = inox --version"
} else {
    $failed++; Write-Host "[FAIL] inox version = inox --version"
}
$helpOutput = & $InoxExe "help" 2>&1 | Out-String
if ($LASTEXITCODE -eq 0 -and $helpOutput.Contains("inox run <file.inox>")) {
    $passed++; Write-Host "[PASS] inox help"
} else {
    $failed++; Write-Host "[FAIL] inox help"; Write-Host "       actual output: $helpOutput"
}
& $InoxExe "run" *> $null
if ($LASTEXITCODE -ne 0) {
    $passed++; Write-Host "[PASS] inox run without a file is an error"
} else {
    $failed++; Write-Host "[FAIL] inox run without a file is an error"
}
# r3.92 (D4): the words after the file are the program's arguments, and the
# status of Halt is kept.
$argsOutput = (& $InoxExe "run" (Join-Path $repoRoot "tests/integration/system/args.inox") "um arg" "dois" 2>$null | Out-String) -replace "`r`n", "`n"
$argsExit = $LASTEXITCODE
if ($argsExit -eq 2 -and $argsOutput.TrimEnd("`n") -eq "1: um arg`n2: dois") {
    $passed++; Write-Host "[PASS] inox run passes arguments and keeps Halt's status"
} else {
    $failed++; Write-Host "[FAIL] inox run passes arguments and keeps Halt's status"; Write-Host "       exit code: $argsExit"; Write-Host "       actual output: $argsOutput"
}
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\typed-ensure.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\typed-ensure.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\rethrow.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\rethrow.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\plain-except.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\plain-except.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\ensure-propagation.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\ensure-propagation.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\retry-success.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\retry-success.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\retry-exhausted.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\retry-exhausted.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\retry-else.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\retry-else.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\retry-zero.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\retry-zero.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\retry-nested.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\retry-nested.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\taxonomy-arithmetic.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\taxonomy-arithmetic.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\taxonomy-range.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\taxonomy-range.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\ensure-control-transfers.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\ensure-control-transfers.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\nested-loop-ensure-transfers.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\nested-loop-ensure-transfers.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\nested-loop-retry-raise.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\nested-loop-retry-raise.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\until-crossing-ensure.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\exceptions\until-crossing-ensure.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\modules\Main.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\modules\Main.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\modules\math-showcase.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\modules\math-showcase.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\modules\generics-across-modules.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\modules\generics-across-modules.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\stdlib\StdMathDemo.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\stdlib\StdMathDemo.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\stdlib\StdMathExpanded.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\stdlib\StdMathExpanded.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\showcase\account-showcase.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\showcase\account-showcase.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\output\variadic-put.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\output\variadic-put.out"))
Invoke-RuntimeTree "tests\integration\input"
Invoke-RuntimeTree "tests\runtime"
Invoke-Examples
Invoke-DiagnosticTree "tests\diagnostics"
Invoke-ModeExitTest `
    -Mode "--emit-llvm" `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\cycles\Cycle.A.inox")) `
    -ExpectSuccess $false

# grammar/grammar.ebnf must agree with the constitution, the lexer and the parser.
$pythonExe = @("python3", "python") | Where-Object {
    $null -ne (Get-Command $_ -ErrorAction SilentlyContinue)
} | Select-Object -First 1
if ($null -eq $pythonExe) {
    Write-Host "[SKIP] grammar/grammar.ebnf consistency (Python 3 not found)"
} else {
    $grammarOutput = & $pythonExe (Join-Path $repoRoot "tools\grammar_consistency.py") $repoRoot 2>&1
    if ($LASTEXITCODE -eq 0) {
        $passed++
        Write-Host "[PASS] grammar/grammar.ebnf consistency"
    } else {
        $failed++
        Write-Host "[FAIL] grammar/grammar.ebnf consistency"
        $grammarOutput | Where-Object { "$_" -match "FAIL|^       " } | ForEach-Object { Write-Host "       $_" }
    }
}

# `inox --version` must match the version and revision at the top of
# openspec/specs/constitution/revisions.md (constitution VERSIONING).
$revisionsText = Get-Content -LiteralPath (Join-Path $repoRoot "openspec\specs\constitution\revisions.md") -Raw
# "Language version: Inox 0.2.0 pre-alpha"; the program prints
# MAJOR.MINOR.RELEASE.BUILD, BUILD being set by each build.
$languageMatch = [regex]::Match($revisionsText, "(?m)^Language version: Inox ([0-9]+\.[0-9]+\.[0-9]+) ([a-z-]+)")
$revisionMatch = [regex]::Match($revisionsText, "(?m)^Canon revision: (r[0-9.]+)")
$expectedVersion = "inox $($languageMatch.Groups[1].Value).<BUILD> $($languageMatch.Groups[2].Value) (canon $($revisionMatch.Groups[1].Value))"
$versionPattern = "^inox " + [regex]::Escape($languageMatch.Groups[1].Value) + "\.[0-9]{1,4} " +
    [regex]::Escape($languageMatch.Groups[2].Value) + " \(canon " + [regex]::Escape($revisionMatch.Groups[1].Value) + "\)$"
$actualVersion = ((& $InoxExe "--version" 2>&1 | Out-String) -replace "`r", "").Trim()
if ($languageMatch.Success -and $revisionMatch.Success -and $actualVersion -match $versionPattern) {
    $passed++
    Write-Host "[PASS] inox --version matches the constitution"
} else {
    $failed++
    Write-Host "[FAIL] inox --version matches the constitution"
    Write-Host "       expected: $expectedVersion"
    Write-Host "       actual: $actualVersion"
}

# Unit tests built next to the compiler: the runtime library (inoxrt-tests)
# and the compiler's own modules (inox-compiler-tests).
foreach ($unit in @(@("inoxrt-tests", "inoxrt unit tests"), @("inox-compiler-tests", "compiler unit tests"))) {
    $unitExe = Join-Path (Split-Path -Parent $InoxExe) ($unit[0] + ".exe")
    if (-not (Test-Path -LiteralPath $unitExe -PathType Leaf)) {
        Write-Host "[SKIP] $($unit[1]) ($($unit[0]).exe not built next to $InoxExe)"
        continue
    }
    $unitOutput = & $unitExe 2>&1
    if ($LASTEXITCODE -eq 0) {
        $passed++
        Write-Host "[PASS] $($unit[1])"
    } else {
        $failed++
        Write-Host "[FAIL] $($unit[1])"
        $unitOutput | ForEach-Object { Write-Host "       $_" }
    }
}

$total = $passed + $failed
Write-Host ""
Write-Host "Summary: $passed passed, $failed failed, $total total"

if ($failed -ne 0) {
    exit 1
}

exit 0
