param(
    # The Release compiler; by default build\Release\inox.exe, or the
    # windows-clang-msvc preset's build\windows-clang-msvc\Release\inox.exe.
    [string] $CompilerExe = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..")

$packageName = "inox-windows-x64"
$distRoot = Join-Path $repoRoot "dist"
$packageRoot = Join-Path $distRoot $packageName
$zipPath = Join-Path $distRoot "$packageName.zip"

$compilerExe = $CompilerExe
if ([string]::IsNullOrWhiteSpace($compilerExe)) {
    $compilerExe = Join-Path $repoRoot "build\Release\inox.exe"
    $presetExe = Join-Path $repoRoot "build\windows-clang-msvc\Release\inox.exe"
    if (!(Test-Path -LiteralPath $compilerExe) -and (Test-Path -LiteralPath $presetExe)) {
        $compilerExe = $presetExe
    }
}

if (!(Test-Path -LiteralPath $compilerExe)) {
    throw "Release compiler not found: $compilerExe. Run: cmake --build build --config Release"
}

function Copy-DirectoryContents {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Source,

        [Parameter(Mandatory = $true)]
        [string] $Destination
    )

    if (!(Test-Path -LiteralPath $Source)) {
        throw "Required directory not found: $Source"
    }

    New-Item -ItemType Directory -Path $Destination -Force | Out-Null

    Get-ChildItem -LiteralPath $Source -Force | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $Destination -Recurse -Force
    }
}

Remove-Item -LiteralPath $packageRoot -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $zipPath -Force -ErrorAction SilentlyContinue

New-Item -ItemType Directory -Path $distRoot -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $packageRoot "bin") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $packageRoot "stdlib") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $packageRoot "examples") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $packageRoot "output") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $packageRoot "docs") -Force | Out-Null

Copy-Item -LiteralPath $compilerExe -Destination (Join-Path $packageRoot "bin\inox.exe") -Force
Copy-DirectoryContents -Source (Join-Path $repoRoot "stdlib") -Destination (Join-Path $packageRoot "stdlib")
Copy-DirectoryContents -Source (Join-Path $repoRoot "examples") -Destination (Join-Path $packageRoot "examples")
# libinoxrt (exceptions, Strings, Files) is found beside the compiler.
$runtimeLib = Join-Path (Split-Path -Parent $compilerExe) "inoxrt.lib"
if (!(Test-Path -LiteralPath $runtimeLib)) {
    throw "inoxrt.lib not found beside the compiler: $runtimeLib"
}
Copy-Item -LiteralPath $runtimeLib -Destination (Join-Path $packageRoot "bin\inoxrt.lib") -Force
# The manual and the constitution (the language law).
Copy-Item -LiteralPath (Join-Path $repoRoot "docs\site\index.html") -Destination (Join-Path $packageRoot "docs\index.html") -Force
New-Item -ItemType Directory -Path (Join-Path $packageRoot "docs\constitution") -Force | Out-Null
foreach ($lawFile in @("spec.md", "engineering.md", "revisions.md")) {
    Copy-Item -LiteralPath (Join-Path $repoRoot "docs\openspec\specs\constitution\$lawFile") -Destination (Join-Path $packageRoot "docs\constitution\$lawFile") -Force
}

if (Test-Path -LiteralPath (Join-Path $repoRoot "licenses")) {
    Copy-DirectoryContents -Source (Join-Path $repoRoot "licenses") -Destination (Join-Path $packageRoot "licenses")
}

foreach ($legalFile in @("LICENSE", "LICENSE.md", "NOTICE.md", "AUTHORS.md", "TRADEMARK.md")) {
    $candidate = Join-Path $repoRoot $legalFile
    if (Test-Path -LiteralPath $candidate) {
        Copy-Item -LiteralPath $candidate -Destination (Join-Path $packageRoot $legalFile) -Force
    }
}

$releaseReadme = Join-Path $repoRoot "docs\release\README.md"
$rootReadme = Join-Path $repoRoot "README.md"

if (Test-Path -LiteralPath $releaseReadme) {
    Copy-Item -LiteralPath $releaseReadme -Destination (Join-Path $packageRoot "README.md") -Force
}
elseif (Test-Path -LiteralPath $rootReadme) {
    Copy-Item -LiteralPath $rootReadme -Destination (Join-Path $packageRoot "README.md") -Force
}
else {
    "# Inox Windows x64 Release" | Set-Content -Path (Join-Path $packageRoot "README.md") -Encoding utf8
}

$envScript = @(
    '$releaseRoot = Split-Path -Parent $MyInvocation.MyCommand.Path',
    '',
    '$env:Path = (Join-Path $releaseRoot "bin") + ";" + $env:Path',
    '$env:INOX_STDLIB = Join-Path $releaseRoot "stdlib"',
    '$env:INOX_OUTPUT_DIR = Join-Path $releaseRoot "output"',
    '',
    'Write-Host "Inox release environment configured."',
    'Write-Host "INOX_STDLIB=$env:INOX_STDLIB"',
    'Write-Host "INOX_OUTPUT_DIR=$env:INOX_OUTPUT_DIR"',
    'Write-Host ("Manual: " + (Join-Path $releaseRoot "docs\index.html"))',
    'Write-Host ("Language law: " + (Join-Path $releaseRoot "docs\constitution\spec.md"))',
    'Write-Host "Compiler:"',
    'Get-Command inox -ErrorAction SilentlyContinue'
)

$envScript | Set-Content -Path (Join-Path $packageRoot "set-inox-env.ps1") -Encoding utf8

"This directory is the default output location for native programs generated from Inox examples when INOX_OUTPUT_DIR points here." |
    Set-Content -Path (Join-Path $packageRoot "output\README.txt") -Encoding utf8

Compress-Archive -Path $packageRoot -DestinationPath $zipPath -Force

Write-Host "Release package created:"
Write-Host $zipPath
