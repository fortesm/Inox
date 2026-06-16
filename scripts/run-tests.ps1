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
    & $InoxExe $TestFile.FullName *> $null
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
    $clang = Get-Command clang -ErrorAction SilentlyContinue
    if ($null -eq $clang) {
        Write-Host "[SKIP] $relativePath link/run (clang not found)"
        return
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

        $clangArgs = @($llPath, "-o", $exePath)
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

function Invoke-RunDriverTest {
    param(
        [System.IO.FileInfo]$TestFile,
        [System.IO.FileInfo]$ExpectedOutputFile
    )

    $relativePath = [System.IO.Path]::GetRelativePath($repoRoot, $TestFile.FullName)
    $clang = Get-Command clang -ErrorAction SilentlyContinue
    if ($null -eq $clang) {
        Write-Host "[SKIP] $relativePath --run (clang not found)"
        return
    }

    $actual = & $InoxExe "--run" $TestFile.FullName 2>&1 | Out-String
    $exitCode = $LASTEXITCODE
    $expected = Get-Content -LiteralPath $ExpectedOutputFile.FullName -Raw
    $actual = ($actual -replace "`r`n", "`n") -replace "`n+$", ""
    $expected = ($expected -replace "`r`n", "`n") -replace "`n+$", ""

    if ($exitCode -eq 0 -and $actual -eq $expected) {
        $script:passed++
        Write-Host "[PASS] $relativePath --run"
    } else {
        $script:failed++
        Write-Host "[FAIL] $relativePath --run"
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

    $actual = Get-Content -LiteralPath $InputFile.FullName | & $InoxExe "--run" $TestFile.FullName 2>&1 | Out-String
    $exitCode = $LASTEXITCODE
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
    -RequiredFragments @('Keyword lexeme="Module" normalized="module"', 'Keyword lexeme="Type" normalized="type"', 'Keyword lexeme="Struct" normalized="struct"', 'IntegerLiteral lexeme="$2A"', 'StringLiteral lexeme="hello"', 'CharLiteral lexeme=', 'Identifier lexeme="End" normalized="end"')

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
    -RequiredFragments @("define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-integer-function.inox")) `
    -RequiredFragments @("define i64 @sum", "%tmp0 = add i64 %a, %b", "ret i64 %tmp0", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-function-call.inox")) `
    -RequiredFragments @("define i64 @sum", "define i64 @double", "%tmp0 = call i64 @sum(i64 %x, i64 %x)", "ret i64 %tmp0", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-local-variables.inox")) `
    -RequiredFragments @("define i64 @compute", "%a = alloca i64", "%b = alloca i64", "store i64 10, ptr %a", "store i64 20, ptr %b", "load i64, ptr %a", "load i64, ptr %b", "add i64", "ret i64", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-inline-typed-local.inox")) `
    -RequiredFragments @("define i64 @compute", "%a = alloca i64", "%b = alloca i64", "store i64 10, ptr %a", "store i64 20, ptr %b", "load i64, ptr %a", "load i64, ptr %b", "add i64", "ret i64", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-local-assignment.inox")) `
    -RequiredFragments @("define i64 @compute", "%a = alloca i64", "%b = alloca i64", "store i64 10, ptr %a", "store i64 20, ptr %b", "add i64", "mul i64", "store i64 %tmp0, ptr %a", "store i64 %tmp3, ptr %b", "ret i64", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-integer-operators.inox")) `
    -RequiredFragments @("define i64 @compute", "%tmp0 = sdiv i64 %a, %b", "srem i64", "shl i64", "ashr i64", "and i64", "or i64", "xor i64", "ret i64", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-bool-comparisons.inox")) `
    -RequiredFragments @("define i1 @isgreater", "define i1 @isequal", "define i1 @isdifferent", "icmp sgt i64", "icmp eq i64", "icmp ne i64", "icmp slt i64", "icmp sle i64", "icmp sge i64", "ret i1", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-bool-operators.inox")) `
    -RequiredFragments @("define i1 @both", "define i1 @either", "define i1 @different", "define i1 @notpositive", "and i1", "or i1", "xor i1", "xor i1 %tmp0, true", "ret i1", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-if-return.inox")) `
    -RequiredFragments @("define i64 @max", "icmp sgt i64", "br i1", "label %then0", "label %else0", "then0:", "else0:", "ret i64", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-if-merge.inox")) `
    -RequiredFragments @("define i64 @maxplusone", "%m = alloca i64", "icmp sgt i64", "br i1", "label %then0", "label %else0", "then0:", "else0:", "br label %endif0", "endif0:", "store i64", "load i64", "add i64", "ret i64", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-while-loop.inox")) `
    -RequiredFragments @("define i64 @sumto", "whilecond0:", "whilebody0:", "whileend0:", "br i1", "br label %whilecond0", "icmp sgt i64", "add i64", "sub i64", "ret i64", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-while-break-continue.inox")) `
    -RequiredFragments @("define i64 @findfirstbelow", "whilecond0:", "whilebody0:", "whileend0:", "br i1", "br label %whilecond0", "br label %whileend0", "icmp eq i64", "sub i64", "store i64", "ret i64", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-if-no-else.inox")) `
    -RequiredFragments @("define i64 @clamppositive", "%x = alloca i64", "icmp slt i64", "br i1", "label %then0", "label %endif0", "then0:", "br label %endif0", "endif0:", "store i64", "load i64", "ret i64", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-elif-return.inox")) `
    -RequiredFragments @("define i64 @compare", "icmp sgt i64", "icmp eq i64", "br i1", "elifcond0_0:", "elifthen0_0:", "ret i64 1", "ret i64 0", "ret i64 -1", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-repeat-flexible-end.inox")) `
    -RequiredFragments @("define i64 @countdown", "repeatbody", "repeatend", "br i1", "br label", "icmp", "ret i64", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-repeat-flexible-start.inox")) `
    -RequiredFragments @("define i64 @countdown", "repeatbody", "repeatcontinue", "repeatend", "br i1", "br label", "icmp", "ret i64", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-repeat-flexible-middle.inox")) `
    -RequiredFragments @("define i64 @countdown", "repeatbody", "repeatcontinue", "repeatend", "br i1", "br label", "icmp", "ret i64", "define i32 @main()", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-repeat-break-continue.inox")) `
    -RequiredFragments @("define i64 @findvalue", "repeatbody", "repeatend", "br i1", "br label", "icmp eq i64", "sub i64", "store i64", "ret i64", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-for-range-break-continue.inox")) `
    -RequiredFragments @("define i64 @sumrange", "forcond", "forbody", "forstep", "forend", "br i1", "br label", "icmp sle i64", "icmp eq i64", "add i64", "store i64", "load i64", "ret i64", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-for-range-step.inox")) `
    -RequiredFragments @("define i64 @sumevenuntil", "forcond", "forbody", "forstep", "forend", "store i64 2, ptr %i", "icmp sle i64", "icmp eq i64", "add i64", ", 2", "br i1", "br label", "ret i64", "define i32 @main()", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-putln-integer.inox")) `
    -RequiredFragments @("@.inox.fmt.i64.nl", "declare i32 @printf", "define i64 @value", "define i32 @main()", "call i32 (ptr, ...) @printf", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-put-output-basic.inox")) `
    -RequiredFragments @("@.inox.fmt.str.nl", "@.inox.fmt.str", "@.inox.true", "@.inox.false", "@.inox.str.", "select i1", "call i32 (ptr, ...) @printf", "define i32 @main()", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-subroutine-calls.inox")) `
    -RequiredFragments @("define i64 @value", "define void @report", "call void @report", "ret void", "report=", "call i32 (ptr, ...) @printf", "define i32 @main()", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-struct-basic.inox")) `
    -RequiredFragments @("%tpoint = type { i64, i64 }", "define i64 @sumpoint", "alloca %tpoint", "zeroinitializer", "getelementptr %tpoint", "store i64 10", "store i64 20", "load i64", "add i64", "call i64 @sumpoint", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-associated-methods.inox")) `
    -RequiredFragments @("%tpoint = type { i64, i64 }", "define void @tpoint.move", "define i64 @tpoint.sum", "ptr %self", "call void @tpoint.move", "call i64 @tpoint.sum", "getelementptr %tpoint", "ret void", "ret i64", "define i32 @main()", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-struct-field-defaults.inox")) `
    -RequiredFragments @("%tconfig = type { i64, i1 }", "define i64 @getport", "alloca %tconfig", "zeroinitializer", "store i64 8080", "store i1 1", "getelementptr %tconfig", "load i64", "call i64 @getport", "ret i32 0")

Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-struct-values.inox")) `
    -RequiredFragments @("%tpoint = type { i64, i64 }", "define %tpoint @makepoint", "define i64 @sumpoint", "define %tpoint @copypoint", "%p.addr = alloca %tpoint", "store %tpoint %p, ptr %p.addr", "load %tpoint", "ret %tpoint", "call %tpoint @makepoint", "call %tpoint @copypoint", "call i64 @sumpoint", "ret i32 0")


Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\codegen\llvm-struct-value-smoke.inox")) `
    -RequiredFragments @("%tpair = type { i64, i64 }", "define %tpair @makepair", "define i64 @sumpair", "call %tpair @makepair", "call i64 @sumpair", "ret i32 0")

Invoke-LinkedExecutionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\output-basic.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\output-basic.out"))

Invoke-BuildDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\run-hello.inox"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\run-hello.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\run-hello.out"))
Invoke-RunDriverTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\modules\Main.inox")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\modules\Main.out"))
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
Invoke-RunDriverInputTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\input\get-integer.inox")) `
    -InputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\input\get-integer.in")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\input\get-integer.out"))
Invoke-RunDriverInputTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\input\getln-two-integers.inox")) `
    -InputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\input\getln-two-integers.in")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\input\getln-two-integers.out"))
Invoke-RunDriverInputTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\input\getln-pause.inox")) `
    -InputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\input\getln-pause.in")) `
    -ExpectedOutputFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\input\getln-pause.out"))
Invoke-ModeExitTest `
    -Mode "--emit-llvm" `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "tests\integration\cycles\Cycle.A.inox")) `
    -ExpectSuccess $false

$total = $passed + $failed
Write-Host ""
Write-Host "Summary: $passed passed, $failed failed, $total total"

if ($failed -ne 0) {
    exit 1
}

exit 0
