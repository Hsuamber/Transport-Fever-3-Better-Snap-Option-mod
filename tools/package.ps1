# Builds the release package from build\ :  Better-Snap-Option-v<version>\  and the .zip of it, in the
# project's 成品 folder (the sibling of this source folder), or in dist\ here when there is no such folder.
#   package.ps1 [-Force] [-OutDir <folder>]
# The version comes from src\plugin\version.h; both DLLs must carry that version in their version resource
# (= made by build.bat from the current sources), unless -Force.  Third-party licenses come from the toolchain
# copies the DLLs were built from.
[CmdletBinding()]
param([switch]$Force, [string]$OutDir)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = Split-Path -Path $PSScriptRoot -Parent
$product = 'Better Snap Option'
$m = Select-String -LiteralPath (Join-Path $root 'src\plugin\version.h') -Pattern '#define TF3SNAP_VERSION "([^"]+)"'
if (-not $m) { throw 'TF3SNAP_VERSION not found in src\plugin\version.h' }
$ver = $m.Matches[0].Groups[1].Value
$build = Join-Path $root 'build'
$toolchain = Join-Path $root 'toolchain'

foreach ($f in 'vulkan-1.dll', 'tf3snap.dll') {
    $p = Join-Path $build $f
    if (-not (Test-Path -LiteralPath $p)) { throw "$p is missing: run build.bat first" }
    $vi = (Get-Item -LiteralPath $p).VersionInfo
    if ($vi.ProductName -ne $product -or $vi.ProductVersion -ne $ver) {
        $msg = "$f is not a $ver build (version resource: '$($vi.ProductName)' '$($vi.ProductVersion)'): run build.bat first"
        if (-not $Force) { throw $msg }
        Write-Warning "$msg  (-Force: packing it anyway)"
    }
}

$name = "Better-Snap-Option-v$ver"
if (-not $OutDir) {
    $sibling = Join-Path (Split-Path -Path $root -Parent) '成品'
    $OutDir = if (Test-Path -LiteralPath $sibling -PathType Container) { $sibling } else { Join-Path $root 'dist' }
}
[void][IO.Directory]::CreateDirectory($OutDir)
$out = Join-Path $OutDir $name
if (Test-Path -LiteralPath $out) { Remove-Item -LiteralPath $out -Recurse -Force }   # our own previous output
foreach ($d in '', 'tools', 'tf3snap', 'licenses') { [void][IO.Directory]::CreateDirectory((Join-Path $out $d)) }

$rel = Join-Path $root 'release'
$copies = @(
    @((Join-Path $rel '安裝-Install.bat'), '安裝-Install.bat'),
    @((Join-Path $rel 'tools\install.ps1'), 'tools\install.ps1'),
    @((Join-Path $rel 'README.txt'), 'README.txt'),
    @((Join-Path $rel 'licenses\THIRD-PARTY.txt'), 'licenses\THIRD-PARTY.txt'),
    @((Join-Path $build 'vulkan-1.dll'), 'vulkan-1.dll'),
    @((Join-Path $build 'tf3snap.dll'), 'tf3snap\tf3snap.dll'),
    @((Join-Path $toolchain 'imgui-1.92.9b\LICENSE.txt'), 'licenses\Dear-ImGui.txt'),
    @((Join-Path $toolchain 'minhook-1.3.4\LICENSE.txt'), 'licenses\MinHook.txt')
)
foreach ($c in $copies) {
    if (-not (Test-Path -LiteralPath $c[0])) { throw "missing $($c[0])" }
    Copy-Item -LiteralPath $c[0] -Destination (Join-Path $out $c[1])
}
$own = Join-Path $root 'LICENSE'
if (Test-Path -LiteralPath $own) { Copy-Item -LiteralPath $own -Destination (Join-Path $out 'LICENSE') }
else { Write-Warning 'No LICENSE file in the project root: the package ships without a license of its own.' }

# SHA256SUMS.txt: one "HASH  relative\path" line per file
$sums = Get-ChildItem -LiteralPath $out -Recurse -File | Sort-Object FullName | ForEach-Object {
    $h = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    '{0}  {1}' -f $h, $_.FullName.Substring($out.Length + 1)
}
[IO.File]::WriteAllLines((Join-Path $out 'SHA256SUMS.txt'), [string[]]$sums, (New-Object Text.UTF8Encoding $false))

# zip with the folder itself at the top, so extracting never spills files into the current folder.  Entries are
# added one by one with '/' separators (ZipFile.CreateFromDirectory on .NET Framework writes '\', which other
# unzip tools take as part of the file name).  No explicit name encoding: then non-ASCII names (安裝-Install.bat)
# are stored as UTF-8 with the language-encoding flag set, which Explorer and 7-Zip read correctly.
$zip = Join-Path $OutDir "$name.zip"
if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::Open($zip, [IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($f in Get-ChildItem -LiteralPath $out -Recurse -File | Sort-Object FullName) {
        $entry = $name + '/' + $f.FullName.Substring($out.Length + 1).Replace('\', '/')
        [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $f.FullName, $entry,
            [IO.Compression.CompressionLevel]::Optimal)
    }
} finally { $archive.Dispose() }

Write-Host "OK: $out"
Write-Host "    $zip  ($('{0:N0}' -f (Get-Item -LiteralPath $zip).Length) bytes)"
