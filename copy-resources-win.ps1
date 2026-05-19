# Mirrors CI's "Copy resources (Windows)" step from .github/workflows/build.yml.
# Run from repo root.

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

$resourceDir = "build/interface/Resources"
New-Item -ItemType Directory -Force -Path "$resourceDir/bin" | Out-Null

# Interface resources (images, shaders)
Copy-Item interface/data/images/* $resourceDir/ -Force
Copy-Item interface/src/renderer/*.glsl $resourceDir/ -Force

# Per-model data files (XML interface defs, default parameters)
Get-ChildItem models/*/data -Directory -ErrorAction SilentlyContinue | ForEach-Object {
    Copy-Item "$($_.FullName)/*" $resourceDir/ -Force
}

# Model binaries
Get-ChildItem models/*/bin -Directory -ErrorAction SilentlyContinue | ForEach-Object {
    Copy-Item "$($_.FullName)/*.exe" "$resourceDir/bin/" -Force -ErrorAction SilentlyContinue
}

# Utility binaries (qmake puts release builds in <util>/release/)
Get-ChildItem build/models/utils/*/release/*.exe -ErrorAction SilentlyContinue | ForEach-Object {
    Copy-Item $_.FullName "$resourceDir/bin/" -Force
}

Write-Output "=== Resources/ ==="
Get-ChildItem $resourceDir | Select-Object Name | Format-Table -AutoSize
Write-Output "=== Resources/bin/ ==="
Get-ChildItem "$resourceDir/bin" | Format-Table Name,Length -AutoSize
