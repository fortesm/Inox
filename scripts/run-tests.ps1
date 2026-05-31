param(
    [string]$InoxExe
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($InoxExe)) {
    $InoxExe = Join-Path $repoRoot "build\Debug\inox.exe"
} elseif (-not [System.IO.Path]::IsPathRooted($InoxExe)) {
    $InoxExe = Join-Path $repoRoot $InoxExe
}

if (-not (Test-Path -LiteralPath $InoxExe -PathType Leaf)) {
    Write-Host "Inox executable not found: $InoxExe"
    Write-Host "Run: cmake --build build"
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

$validExamples = Get-ChildItem -LiteralPath (Join-Path $repoRoot "examples") -Filter "*.inox" -File |
    Sort-Object Name
$invalidTests = Get-ChildItem -LiteralPath (Join-Path $repoRoot "tests\invalid") -Filter "*.inox" -File |
    Sort-Object Name

foreach ($example in $validExamples) {
    Invoke-InoxTest -TestFile $example -ExpectSuccess $true
}

foreach ($test in $invalidTests) {
    Invoke-InoxTest -TestFile $test -ExpectSuccess $false
}

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
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-local-assignment.inox")) `
    -RequiredFragments @("define i64 @compute", "%a = alloca i64", "%b = alloca i64", "store i64 10, ptr %a", "store i64 20, ptr %b", "add i64", "mul i64", "store i64 %tmp2, ptr %a", "store i64 %tmp4, ptr %b", "ret i64", "define i32 @main()", "ret i32 0")
Invoke-LlvmEmissionTest `
    -TestFile (Get-Item -LiteralPath (Join-Path $repoRoot "examples\llvm-integer-operators.inox")) `
    -RequiredFragments @("define i64 @compute", "%tmp0 = sdiv i64 %a, %b", "%tmp1 = sdiv i64 %a, %b", "srem i64", "shl i64", "ashr i64", "and i64", "or i64", "xor i64", "ret i64", "define i32 @main()", "ret i32 0")
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

$total = $passed + $failed
Write-Host ""
Write-Host "Summary: $passed passed, $failed failed, $total total"

if ($failed -ne 0) {
    exit 1
}

exit 0
