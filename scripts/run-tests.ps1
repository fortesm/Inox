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

$total = $passed + $failed
Write-Host ""
Write-Host "Summary: $passed passed, $failed failed, $total total"

if ($failed -ne 0) {
    exit 1
}

exit 0
