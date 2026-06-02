$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..")

$packageName = "inox-windows-x64"
$distRoot = Join-Path $repoRoot "dist"
$packageRoot = Join-Path $distRoot $packageName
$zipPath = Join-Path $distRoot "$packageName.zip"

$compilerExe = Join-Path $repoRoot "build\Release\inox.exe"

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

Copy-Item -LiteralPath $compilerExe -Destination (Join-Path $packageRoot "bin\inox.exe") -Force

Copy-DirectoryContents -Source (Join-Path $repoRoot "stdlib") -Destination (Join-Path $packageRoot "stdlib")
Copy-DirectoryContents -Source (Join-Path $repoRoot "examples") -Destination (Join-Path $packageRoot "examples")

if (Test-Path -LiteralPath (Join-Path $repoRoot "licenses")) {
    Copy-DirectoryContents -Source (Join-Path $repoRoot "licenses") -Destination (Join-Path $packageRoot "licenses")
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
    "" | Add-Content -Path (Join-Path $packageRoot "README.md") -Encoding utf8
    "This package contains a prebuilt Inox compiler, the standard library, examples, and a default output directory." | Add-Content -Path (Join-Path $packageRoot "README.md") -Encoding utf8
    "" | Add-Content -Path (Join-Path $packageRoot "README.md") -Encoding utf8
    "Run .\set-inox-env.ps1 before testing the compiler." | Add-Content -Path (Join-Path $packageRoot "README.md") -Encoding utf8
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
    'Write-Host "Compiler:"',
    'Get-Command inox -ErrorAction SilentlyContinue'
)

$envScript | Set-Content -Path (Join-Path $packageRoot "set-inox-env.ps1") -Encoding utf8

"This directory is the default output location for native programs generated from Inox examples when INOX_OUTPUT_DIR points here." |
    Set-Content -Path (Join-Path $packageRoot "output\README.txt") -Encoding utf8

Compress-Archive -Path $packageRoot -DestinationPath $zipPath -Force

Write-Host "Release package created:"
Write-Host $zipPath
