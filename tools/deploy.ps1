# Developer shortcut behind deploy.bat: installs what build.bat made (build\*.dll) with the release installer.
#   deploy.ps1 [-GameDir <folder>] [-Action Install|Uninstall|Report] [-Yes] [-RemoveData]
# The game folder is remembered in deploy.local.txt next to deploy.bat (UTF-8, one line; not shipped).  Without
# it, and without -GameDir, it asks once.  The source tree no longer lives inside the game folder, so the
# installer cannot find the game by walking up from build\ any more.
[CmdletBinding()]
param(
    [string]$GameDir,
    [string]$Action = 'Install',
    [switch]$Yes,
    [switch]$RemoveData
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = Split-Path -Path $PSScriptRoot -Parent
$saved = Join-Path $root 'deploy.local.txt'
$exe = 'TransportFever3.exe'
$utf8 = New-Object Text.UTF8Encoding $false

if (-not $GameDir -and (Test-Path -LiteralPath $saved -PathType Leaf)) {
    $GameDir = [IO.File]::ReadAllText($saved, $utf8)
    Write-Host "Game folder (from deploy.local.txt): $($GameDir.Trim())"
}
if (-not $GameDir) {
    $GameDir = Read-Host "Game folder (the one with $exe; you can drag it into this window)"
}
$GameDir = $GameDir.Trim().Trim('"').TrimEnd('\')
if ((Test-Path -LiteralPath $GameDir -PathType Leaf) -and (Split-Path -Path $GameDir -Leaf) -ieq $exe) {
    $GameDir = Split-Path -Path $GameDir -Parent   # the exe itself was dragged in
}
if (-not $GameDir -or -not (Test-Path -LiteralPath (Join-Path $GameDir $exe) -PathType Leaf)) {
    throw "No $exe in '$GameDir'.  Fix or delete $saved, or pass -GameDir."
}
$GameDir = (Resolve-Path -LiteralPath $GameDir).ProviderPath.TrimEnd('\')
[IO.File]::WriteAllText($saved, $GameDir + "`r`n", $utf8)

$a = @{ Action = $Action; GameDir = $GameDir; SourceRoot = (Join-Path $root 'build') }
if ($Yes) { $a.Yes = $true }
if ($RemoveData) { $a.RemoveData = $true }
& (Join-Path $root 'release\tools\install.ps1') @a
