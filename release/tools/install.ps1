# Better Snap Option (0.2.x: TF3 Snap Options) - installer: install / update / uninstall / error report.
# Started by the .bat next to the tools folder.  Without arguments it asks; with arguments it runs unattended:
#   install.ps1 -Action Install|Uninstall|Report [-GameDir <folder>] [-SourceRoot <package or build folder>]
#               [-Yes] [-RemoveData]
# Keep this file UTF-8 with BOM: Windows PowerShell 5.1 reads a BOM-less file in the ANSI code page.
#
# In the game folder it only touches vulkan-1.dll, tf3snap\tf3snap.dll, tf3snap\install.json and, when another
# vulkan-1.dll was there before us, tf3snap\backup\ (put back on uninstall).  Settings, panel text files and logs
# in tf3snap\ are never overwritten; uninstall deletes them only when asked.  It never starts or stops the game.
# Any failure, and menu choice 3, writes 報錯.log next to the .bat for bug reports.
[CmdletBinding()]
param(
    [string]$GameDir,
    [string]$Action,
    [string]$SourceRoot,
    [switch]$Yes,
    [switch]$RemoveData
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$Product = 'Better Snap Option'
$LegacyProducts = @('TF3 Snap Options')   # earlier names (0.2.2) in version resources and install.json
$AppId = '3493540'
$GameExe = 'TransportFever3.exe'
$ReportName = '報錯.log'
$Latin1 = [Text.Encoding]::GetEncoding(28591)   # byte <-> char 1:1, for searching binaries
# Builds from before the version resource (0.2.1 and older) are recognised by a string only they contain.
$Markers = @{
    'vulkan-1.dll'        = $Latin1.GetString([Text.Encoding]::Unicode.GetBytes('tf3snap\tf3snap.dll'))
    'tf3snap\tf3snap.dll' = 'TF3 Snap Options v%s starting'
}

# ---- messages ------------------------------------------------------------------------------------------
$Msg = @{
'zh-TW' = @{
    title = '更好的吸附貼齊 Better Snap Option {0}：安裝 / 更新 / 移除'
    menu1 = '  1 = 安裝或更新（預設）'
    menu2 = '  2 = 移除'
    menu3 = '  3 = 產生報錯檔（遊戲裡看不到面板、遊戲崩潰時用）'
    choice = '請選擇 [1]'
    badChoice = '無效的選項：{0}'
    found = '找到遊戲資料夾：{0}'
    askDirDefault = '按 Enter 使用這個資料夾，或輸入 / 拖入其他資料夾'
    askDir = '請輸入遊戲資料夾（含 TransportFever3.exe；可直接把資料夾拖進這個視窗）'
    noDir = '沒有指定遊戲資料夾。'
    notGame = '這不是 Transport Fever 3 的遊戲資料夾（找不到 TransportFever3.exe）：{0}'
    running = '遊戲還在執行（行程 ID：{0}）。請完全關閉遊戲後再試一次。'
    badPayload = '安裝包裡的檔案缺少或損毀：{0}。請重新下載並完整解壓。'
    versions = '目前安裝：{0}  →  安裝包：{1}'
    notInstalled = '未安裝'
    oldVer = '舊版（版本不明）'
    foreign = '遊戲資料夾裡已有別的 vulkan-1.dll（不是本外掛的，可能屬於其他模組）：{0}'
    foreignInfo = '  那個檔案：{0}'
    foreignBackup = '它會先備份到 {0}，移除本外掛時自動還原。安裝本外掛期間，那個模組無法運作。'
    confirm = '要繼續嗎？(Y/N)'
    cancelled = '已取消，沒有改動任何檔案。'
    backedUp = '備份：{0}'
    write = '寫入：{0}'
    same = '已是最新，略過：{0}'
    verifyFailed = '複製後檢查不符：{0}'
    rolledBack = '已還原成安裝前的檔案。'
    rollbackFailed = '無法還原 {0}：{1}'
    disabledWarn = '注意：外掛目前被停用（{0} 存在），刪掉這個檔案才會載入。'
    installed = '完成：已安裝 {0}。啟動遊戲並打開道路或鐵路工具，面板就會出現。'
    keepData = '設定與面板文字檔都保留在 {0}'
    askRemoveData = '要一併刪除設定、面板文字檔與紀錄（tf3snap 資料夾）嗎？(y/N)'
    remove = '刪除：{0}'
    restore = '還原原本的檔案：{0}'
    restoreSkipped = '沒有自動還原備份（vulkan-1.dll 已被換過，或備份內容不符），備份保留在：{0}'
    notOurs = '{0} 不是本外掛的檔案，保留不動。'
    nothing = '這個遊戲資料夾沒有安裝本外掛。'
    removed = '完成：已移除本外掛（設定與文字檔保留在 {0}）。'
    removedAll = '完成：已移除本外掛與所有設定。'
    failed = '失敗：{0}'
    admin = '沒有寫入權限：請對安裝用的 .bat 按右鍵 →「以系統管理員身分執行」。'
    reportWritten = '已產生報錯檔：{0}'
    reportHint = '回報問題時請附上這個檔案。檔案裡有電腦上的資料夾路徑（可能含使用者名稱），公開貼出前可以先看一下。'
    reportFailed = '無法寫入報錯檔：{0}'
    badRecord = '安裝紀錄 tf3snap\install.json 讀不懂，忽略。'
}
'zh-CN' = @{
    title = '更好的吸附贴齐 Better Snap Option {0}：安装 / 更新 / 卸载'
    menu1 = '  1 = 安装或更新（默认）'
    menu2 = '  2 = 卸载'
    menu3 = '  3 = 生成报错文件（游戏中看不到面板、游戏崩溃时使用）'
    choice = '请选择 [1]'
    badChoice = '无效的选项：{0}'
    found = '找到游戏目录：{0}'
    askDirDefault = '按 Enter 使用此目录，或输入 / 拖入其他目录'
    askDir = '请输入游戏目录（含 TransportFever3.exe；可直接把文件夹拖进此窗口）'
    noDir = '未指定游戏目录。'
    notGame = '这不是 Transport Fever 3 的游戏目录（找不到 TransportFever3.exe）：{0}'
    running = '游戏仍在运行（进程 ID：{0}）。请完全退出游戏后重试。'
    badPayload = '安装包中的文件缺失或损坏：{0}。请重新下载并完整解压。'
    versions = '当前安装：{0}  →  安装包：{1}'
    notInstalled = '未安装'
    oldVer = '旧版（版本未知）'
    foreign = '游戏目录中已有其他 vulkan-1.dll（不属于本插件，可能来自其他模组）：{0}'
    foreignInfo = '  该文件：{0}'
    foreignBackup = '它会先备份到 {0}，卸载本插件时自动还原。安装本插件期间，该模组无法工作。'
    confirm = '要继续吗？(Y/N)'
    cancelled = '已取消，未改动任何文件。'
    backedUp = '备份：{0}'
    write = '写入：{0}'
    same = '已是最新，跳过：{0}'
    verifyFailed = '复制后校验不符：{0}'
    rolledBack = '已还原为安装前的文件。'
    rollbackFailed = '无法还原 {0}：{1}'
    disabledWarn = '注意：插件当前被停用（存在 {0}），删除该文件后才会加载。'
    installed = '完成：已安装 {0}。启动游戏并打开道路或铁路工具，面板就会出现。'
    keepData = '设置与面板文字文件均保留在 {0}'
    askRemoveData = '要一并删除设置、面板文字文件与日志（tf3snap 文件夹）吗？(y/N)'
    remove = '删除：{0}'
    restore = '还原原来的文件：{0}'
    restoreSkipped = '未自动还原备份（vulkan-1.dll 已被替换，或备份内容不符），备份保留在：{0}'
    notOurs = '{0} 不是本插件的文件，保留不动。'
    nothing = '此游戏目录未安装本插件。'
    removed = '完成：已卸载本插件（设置与文字文件保留在 {0}）。'
    removedAll = '完成：已卸载本插件及所有设置。'
    failed = '失败：{0}'
    admin = '没有写入权限：请右键点击安装用的 .bat →“以管理员身份运行”。'
    reportWritten = '已生成报错文件：{0}'
    reportHint = '反馈问题时请附上此文件。文件中含有电脑上的文件夹路径（可能包含用户名），公开发布前可以先检查一下。'
    reportFailed = '无法写入报错文件：{0}'
    badRecord = '安装记录 tf3snap\install.json 无法读取，已忽略。'
}
'en' = @{
    title = 'Better Snap Option {0}: install / update / uninstall'
    menu1 = '  1 = Install or update (default)'
    menu2 = '  2 = Uninstall'
    menu3 = '  3 = Write an error report (panel not showing in the game, crashes)'
    choice = 'Choose [1]'
    badChoice = 'Invalid choice: {0}'
    found = 'Found the game folder: {0}'
    askDirDefault = 'Press Enter to use it, or type / drag in another folder'
    askDir = 'Game folder (the one with TransportFever3.exe; you can drag the folder into this window)'
    noDir = 'No game folder given.'
    notGame = 'Not a Transport Fever 3 folder (no TransportFever3.exe): {0}'
    running = 'The game is still running (process ID {0}). Close it completely and try again.'
    badPayload = 'A file of the package is missing or damaged: {0}. Download it again and extract everything.'
    versions = 'Installed: {0}  ->  package: {1}'
    notInstalled = 'not installed'
    oldVer = 'older build (version unknown)'
    foreign = 'The game folder already has a different vulkan-1.dll (not ours, probably another mod''s): {0}'
    foreignInfo = '  That file: {0}'
    foreignBackup = 'It will be backed up to {0} and put back when you uninstall. While this plugin is installed, that mod will not work.'
    confirm = 'Continue? (Y/N)'
    cancelled = 'Cancelled; nothing was changed.'
    backedUp = 'Backed up: {0}'
    write = 'Wrote: {0}'
    same = 'Already up to date: {0}'
    verifyFailed = 'Copy verification failed: {0}'
    rolledBack = 'The previous files were put back.'
    rollbackFailed = 'Could not put back {0}: {1}'
    disabledWarn = 'Note: the plugin is disabled ({0} exists); delete that file to load it.'
    installed = 'Done: {0} is installed. Start the game and open the road or track tool to see the panel.'
    keepData = 'Settings and panel text files are kept in {0}'
    askRemoveData = 'Also delete settings, panel text files and logs (the tf3snap folder)? (y/N)'
    remove = 'Removed: {0}'
    restore = 'Put back the original: {0}'
    restoreSkipped = 'The backup was not restored automatically (vulkan-1.dll was replaced, or the backup does not match); it is kept at: {0}'
    notOurs = '{0} is not ours; left alone.'
    nothing = 'This plugin is not installed in that game folder.'
    removed = 'Done: the plugin is uninstalled (settings and text files kept in {0}).'
    removedAll = 'Done: the plugin and all its settings are removed.'
    failed = 'Failed: {0}'
    admin = 'No write permission: right-click the installer .bat and choose "Run as administrator".'
    reportWritten = 'Error report written: {0}'
    reportHint = 'Attach this file when you report the problem. It contains folder paths from this PC (possibly your user name); check it before posting it publicly.'
    reportFailed = 'Could not write the error report: {0}'
    badRecord = 'Could not read tf3snap\install.json; ignoring it.'
}
'ja' = @{
    title = 'Better Snap Option {0}：インストール / 更新 / アンインストール'
    menu1 = '  1 = インストール / 更新（既定）'
    menu2 = '  2 = アンインストール'
    menu3 = '  3 = エラーレポートを作成（ゲーム内にパネルが出ない・クラッシュしたとき）'
    choice = '番号を選んでください [1]'
    badChoice = '無効な選択です：{0}'
    found = 'ゲームフォルダーが見つかりました：{0}'
    askDirDefault = 'Enter でこのフォルダーを使用します。別のフォルダーは入力するかドラッグしてください'
    askDir = 'ゲームフォルダー（TransportFever3.exe があるフォルダー。このウィンドウにドラッグしても構いません）'
    noDir = 'ゲームフォルダーが指定されていません。'
    notGame = 'Transport Fever 3 のフォルダーではありません（TransportFever3.exe がありません）：{0}'
    running = 'ゲームが実行中です（プロセス ID：{0}）。完全に終了してからもう一度実行してください。'
    badPayload = 'パッケージのファイルが見つからないか壊れています：{0}。もう一度ダウンロードしてすべて展開してください。'
    versions = 'インストール済み：{0}  →  パッケージ：{1}'
    notInstalled = '未インストール'
    oldVer = '旧版（バージョン不明）'
    foreign = 'ゲームフォルダーに別の vulkan-1.dll があります（本プラグインのものではなく、他の MOD の可能性があります）：{0}'
    foreignInfo = '  そのファイル：{0}'
    foreignBackup = '{0} にバックアップし、アンインストール時に自動で元に戻します。本プラグインをインストールしている間、その MOD は動作しません。'
    confirm = '続行しますか？(Y/N)'
    cancelled = 'キャンセルしました。何も変更していません。'
    backedUp = 'バックアップ：{0}'
    write = '書き込み：{0}'
    same = '最新のためスキップ：{0}'
    verifyFailed = 'コピー後の検証に失敗しました：{0}'
    rolledBack = 'インストール前のファイルに戻しました。'
    rollbackFailed = '{0} を元に戻せませんでした：{1}'
    disabledWarn = '注意：プラグインは無効になっています（{0} があります）。読み込むにはこのファイルを削除してください。'
    installed = '完了：{0} をインストールしました。ゲームを起動して道路または線路ツールを開くとパネルが表示されます。'
    keepData = '設定とパネルの文字ファイルは {0} に残しています'
    askRemoveData = '設定、パネルの文字ファイル、ログ（tf3snap フォルダー）も削除しますか？(y/N)'
    remove = '削除：{0}'
    restore = '元のファイルを戻しました：{0}'
    restoreSkipped = 'バックアップは自動で戻していません（vulkan-1.dll が置き換えられたか、バックアップが一致しません）。保存先：{0}'
    notOurs = '{0} は本プラグインのファイルではないため、そのままにしています。'
    nothing = 'このゲームフォルダーには本プラグインがインストールされていません。'
    removed = '完了：プラグインをアンインストールしました（設定と文字ファイルは {0} に残しています）。'
    removedAll = '完了：プラグインとすべての設定を削除しました。'
    failed = '失敗しました：{0}'
    admin = '書き込み権限がありません：インストール用の .bat を右クリックして「管理者として実行」を選んでください。'
    reportWritten = 'エラーレポートを作成しました：{0}'
    reportHint = '問題を報告するときはこのファイルを添付してください。PC のフォルダーパス（ユーザー名を含む場合があります）が含まれているので、公開する前に確認してください。'
    reportFailed = 'エラーレポートを書き込めませんでした：{0}'
    badRecord = 'インストール記録 tf3snap\install.json を読み込めないため無視します。'
}
}

# Language: the panel's own setting when the game folder is already known (see main), else Windows' display
# language - the same rule the panel uses on a first run.
$ui = (Get-UICulture).Name
$Lang = switch -Regex ($ui) {
    '^zh-(TW|HK|MO)|^zh-Hant' { 'zh-TW'; break }
    '^zh' { 'zh-CN'; break }
    '^ja' { 'ja'; break }
    default { 'en' }
}

# The panel language saved in an existing installation: tf3snap.ini lang=, and an ini from 0.2.1 or older
# (no lang=) means Traditional Chinese, as in the panel.  $null when there is no installation.
function Get-PanelLang([string]$Dir) {
    if (-not $Dir) { return $null }
    $ini = Join-Path $Dir 'tf3snap\tf3snap.ini'
    if (-not (Test-Path -LiteralPath $ini -PathType Leaf)) { return $null }
    $m = [regex]::Match([IO.File]::ReadAllText($ini), '(?m)^\s*lang\s*=\s*(zh-TW|zh-CN|en|ja)\s*$')
    if ($m.Success) { return $m.Groups[1].Value }
    return 'zh-TW'
}

function L([string]$Id) {
    $t = $Msg[$Lang][$Id]
    if (-not $t) { $t = $Msg['en'][$Id] }
    if (-not $t) { $t = $Id }
    if ($args.Count) { $t = $t -f $args }
    return $t
}

# Everything shown is also kept for the error report.
$script:Log = New-Object 'System.Collections.Generic.List[string]'
function Say([string]$Text, [string]$Color = '') {
    $script:Log.Add(('[{0:HH:mm:ss}] {1}' -f (Get-Date), $Text))
    if ($Color) { Write-Host $Text -ForegroundColor $Color } else { Write-Host $Text }
}
function Ask([string]$Prompt) {
    $a = Read-Host $Prompt
    if ($null -eq $a) { $a = '' }
    $script:Log.Add(('[{0:HH:mm:ss}] {1} > {2}' -f (Get-Date), $Prompt, $a))
    return $a.Trim()
}

# ---- helpers ------------------------------------------------------------------------------------------
function Get-Sha([string]$Path) {
    $stream = [IO.File]::Open($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '') }
    finally { $sha.Dispose(); $stream.Dispose() }
}

function Prop($Obj, [string]$Name) {
    if ($null -eq $Obj) { return $null }
    $p = $Obj.PSObject.Properties[$Name]
    if ($p) { return $p.Value }
    return $null
}

function Test-Dll64([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $false }
    $b = [IO.File]::ReadAllBytes($Path)
    if ($b.Length -lt 512 -or $b[0] -ne 0x4d -or $b[1] -ne 0x5a) { return $false }
    $pe = [BitConverter]::ToInt32($b, 0x3c)
    if ($pe -lt 64 -or $pe -gt ($b.Length - 26)) { return $false }
    return ([BitConverter]::ToUInt32($b, $pe) -eq 0x4550 -and [BitConverter]::ToUInt16($b, $pe + 4) -eq 0x8664 -and
            ([BitConverter]::ToUInt16($b, $pe + 22) -band 0x2000))
}

function Get-VerInfo([string]$Path) {
    try { return (Get-Item -LiteralPath $Path).VersionInfo } catch { return $null }
}

# Ours: version resource says so (0.2.2+), or the file carries the marker string of the older builds.
function Test-Ours([string]$Path, [string]$Rel) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $false }
    $v = Get-VerInfo $Path
    if ($v -and ($v.ProductName -eq $Product -or $LegacyProducts -contains $v.ProductName)) { return $true }
    try { $text = $Latin1.GetString([IO.File]::ReadAllBytes($Path)) } catch { return $false }
    return $text.IndexOf($Markers[$Rel], [StringComparison]::Ordinal) -ge 0
}

function Get-Version([string]$Path) {
    $v = Get-VerInfo $Path
    if ($v -and $v.ProductVersion) { return $v.ProductVersion.Trim() }
    return (L 'oldVer')
}

function Describe-File([string]$Path) {
    $i = Get-Item -LiteralPath $Path
    $v = $i.VersionInfo
    $parts = @(@($v.ProductName, $v.FileDescription, $v.FileVersion) | Where-Object { $_ })
    $what = if ($parts.Count) { $parts -join ' / ' } else { '(no version info)' }
    return ('{0}; {1:N0} bytes; {2:yyyy-MM-dd HH:mm}' -f $what, $i.Length, $i.LastWriteTime)
}

function Get-SteamPath {
    try { $p = (Get-ItemProperty -Path 'HKCU:\Software\Valve\Steam' -ErrorAction Stop).SteamPath } catch { return $null }
    if ($p) { return ($p -replace '/', '\') }
    return $null
}

# Game folder: an ancestor of the package (it was extracted inside the game folder), else every Steam library
# that has the game.  deploy.bat passes -GameDir (remembered in deploy.local.txt).
function Find-GameDir([string]$From) {
    $cand = New-Object 'System.Collections.Generic.List[string]'
    $d = $From
    for ($i = 0; $i -lt 4 -and $d; ++$i) { $cand.Add($d); $d = Split-Path -Path $d -Parent }
    $steam = Get-SteamPath
    if ($steam) {
        $libs = New-Object 'System.Collections.Generic.List[string]'
        $libs.Add($steam)
        $vdf = Join-Path $steam 'steamapps\libraryfolders.vdf'
        if (Test-Path -LiteralPath $vdf) {
            foreach ($m in [regex]::Matches([IO.File]::ReadAllText($vdf), '"path"\s+"([^"]+)"')) {
                $libs.Add($m.Groups[1].Value.Replace('\\', '\'))
            }
        }
        foreach ($lib in $libs) {
            $dir = 'Transport Fever 3'
            $acf = Join-Path $lib "steamapps\appmanifest_$AppId.acf"
            if (Test-Path -LiteralPath $acf) {
                $m = [regex]::Match([IO.File]::ReadAllText($acf), '"installdir"\s+"([^"]+)"')
                if ($m.Success) { $dir = $m.Groups[1].Value }
            }
            $cand.Add((Join-Path $lib "steamapps\common\$dir"))
        }
    }
    foreach ($c in $cand) {
        if ($c -and (Test-Path -LiteralPath (Join-Path $c $GameExe) -PathType Leaf)) {
            return (Resolve-Path -LiteralPath $c).ProviderPath.TrimEnd('\')
        }
    }
    return $null
}

function Resolve-GameDir([string]$Dir) {
    $Dir = $Dir.Trim().Trim('"')
    if (-not $Dir) { throw (L 'noDir') }
    if ((Test-Path -LiteralPath $Dir -PathType Leaf) -and (Split-Path -Path $Dir -Leaf) -ieq $GameExe) {
        $Dir = Split-Path -Path $Dir -Parent   # the exe itself was dragged in
    }
    if (-not (Test-Path -LiteralPath (Join-Path $Dir $GameExe) -PathType Leaf)) { throw (L 'notGame' $Dir) }
    return (Resolve-Path -LiteralPath $Dir).ProviderPath.TrimEnd('\')
}

function Assert-GameStopped {
    $p = @(Get-Process -Name ([IO.Path]::GetFileNameWithoutExtension($GameExe)) -ErrorAction SilentlyContinue)
    if ($p.Count) { throw (L 'running' (($p | ForEach-Object { $_.Id }) -join ', ')) }
}

# Package layout: vulkan-1.dll + tf3snap\tf3snap.dll.  A build\ folder (both DLLs side by side) works too.
function Get-Payload([string]$Root) {
    $plugin = Join-Path $Root 'tf3snap\tf3snap.dll'
    if (-not (Test-Path -LiteralPath $plugin)) { $plugin = Join-Path $Root 'tf3snap.dll' }
    $items = @(
        @{ Rel = 'vulkan-1.dll'; Src = (Join-Path $Root 'vulkan-1.dll') },
        @{ Rel = 'tf3snap\tf3snap.dll'; Src = $plugin }
    )
    foreach ($p in $items) {
        if (-not (Test-Dll64 $p.Src) -or -not (Test-Ours $p.Src $p.Rel)) { throw (L 'badPayload' $p.Src) }
        $p.Hash = Get-Sha $p.Src
    }
    return $items
}

function Read-Record {
    $p = Join-Path $script:Game 'tf3snap\install.json'
    if (-not (Test-Path -LiteralPath $p)) { return $null }
    try {
        $r = [IO.File]::ReadAllText($p) | ConvertFrom-Json
        $name = Prop $r 'product'
        if ($name -eq $Product -or $LegacyProducts -contains $name) { return $r }
    } catch { }
    Say (L 'badRecord') 'Yellow'
    return $null
}

function Write-Record($Rec) {
    $p = Join-Path $script:Game 'tf3snap\install.json'
    [IO.File]::WriteAllText($p, ($Rec | ConvertTo-Json -Depth 5), (New-Object Text.UTF8Encoding $false))
}

# ---- install / update -------------------------------------------------------------------------------
function Invoke-Install {
    $payload = @(Get-Payload $script:Source)
    $pkgVer = Get-Version $payload[1].Src
    $curPlugin = Join-Path $script:Game 'tf3snap\tf3snap.dll'
    $curVer = if (Test-Ours $curPlugin 'tf3snap\tf3snap.dll') { Get-Version $curPlugin } else { L 'notInstalled' }
    Say (L 'versions' $curVer $pkgVer) 'Cyan'

    $old = Read-Record
    $foreign = $null
    $oldForeign = Prop $old 'foreign'
    if ($oldForeign) { $foreign = [ordered]@{ backup = $oldForeign.backup; sha256 = $oldForeign.sha256 } }

    # Another mod's vulkan-1.dll: keep a verified copy, record it, put it back on uninstall.
    $dstProxy = Join-Path $script:Game 'vulkan-1.dll'
    $foreignNow = (Test-Path -LiteralPath $dstProxy -PathType Leaf) -and -not (Test-Ours $dstProxy 'vulkan-1.dll')
    $backupRel = 'tf3snap\backup\vulkan-1.{0:yyyyMMdd-HHmmss}.dll' -f (Get-Date)
    if ($foreignNow) {
        Say (L 'foreign' $dstProxy) 'Yellow'
        Say (L 'foreignInfo' (Describe-File $dstProxy)) 'Yellow'
        Say (L 'foreignBackup' (Join-Path $script:Game $backupRel)) 'Yellow'
        if (-not $Yes) {
            if ((Ask (L 'confirm')) -notmatch '^[yY]') { Say (L 'cancelled'); return }
        }
    }

    Assert-GameStopped
    [void][IO.Directory]::CreateDirectory((Join-Path $script:Game 'tf3snap'))
    if ($foreignNow) {
        $b = Join-Path $script:Game $backupRel
        [void][IO.Directory]::CreateDirectory((Split-Path -Path $b -Parent))
        Copy-Item -LiteralPath $dstProxy -Destination $b
        $h = Get-Sha $b
        if ($h -ne (Get-Sha $dstProxy)) { throw (L 'verifyFailed' $b) }
        $foreign = [ordered]@{ backup = $backupRel; sha256 = $h }
        Say (L 'backedUp' $b)
    }

    # Stage every file next to its destination (verified), then swap them in; undo the swaps on any error.
    $work = New-Object 'System.Collections.Generic.List[object]'
    foreach ($p in $payload) {
        $p.Dst = Join-Path $script:Game $p.Rel
        if ((Test-Path -LiteralPath $p.Dst -PathType Leaf) -and (Get-Sha $p.Dst) -eq $p.Hash) { Say (L 'same' $p.Dst); continue }
        $p.New = $p.Dst + '.tf3new'
        $p.Old = $p.Dst + '.tf3old'
        $p.HadOld = $false
        $p.Done = $false
        $work.Add($p)
    }
    try {
        foreach ($p in $work) {
            Copy-Item -LiteralPath $p.Src -Destination $p.New -Force
            if ((Get-Sha $p.New) -ne $p.Hash) { throw (L 'verifyFailed' $p.Dst) }
        }
        Assert-GameStopped
        foreach ($p in $work) {
            if (Test-Path -LiteralPath $p.Dst) { Move-Item -LiteralPath $p.Dst -Destination $p.Old -Force; $p.HadOld = $true }
            Move-Item -LiteralPath $p.New -Destination $p.Dst
            $p.Done = $true
            Say (L 'write' $p.Dst)
        }
    } catch {
        for ($i = $work.Count - 1; $i -ge 0; --$i) {
            $p = $work[$i]
            try {
                if ($p.Done -and (Test-Path -LiteralPath $p.Dst)) { Remove-Item -LiteralPath $p.Dst -Force }
                if ($p.HadOld) { Move-Item -LiteralPath $p.Old -Destination $p.Dst -Force }
            } catch { Say (L 'rollbackFailed' $p.Dst $_.Exception.Message) 'Red' }
        }
        if (@($work | Where-Object { $_.HadOld -or $_.Done }).Count) { Say (L 'rolledBack') 'Yellow' }
        throw
    } finally {
        foreach ($p in $work) {
            if (Test-Path -LiteralPath $p.New) { Remove-Item -LiteralPath $p.New -Force -ErrorAction SilentlyContinue }
        }
    }
    foreach ($p in $work) {
        if (Test-Path -LiteralPath $p.Old) { Remove-Item -LiteralPath $p.Old -Force -ErrorAction SilentlyContinue }
    }

    $rec = [ordered]@{
        product = $Product; schema = 1; version = $pkgVer; installed_at = (Get-Date).ToString('o')
        files = @($payload | ForEach-Object { [ordered]@{ path = $_.Rel; sha256 = $_.Hash } })
    }
    if ($foreign) { $rec.foreign = $foreign }
    Write-Record $rec

    $disabled = Join-Path $script:Game 'tf3snap\disabled'
    if (Test-Path -LiteralPath $disabled) { Say (L 'disabledWarn' $disabled) 'Yellow' }
    Say (L 'keepData' (Join-Path $script:Game 'tf3snap'))
    Say (L 'installed' $pkgVer) 'Green'
}

# ---- uninstall -----------------------------------------------------------------------------------------
function Invoke-Uninstall {
    $rec = Read-Record
    $proxy = Join-Path $script:Game 'vulkan-1.dll'
    $plugin = Join-Path $script:Game 'tf3snap\tf3snap.dll'
    $dir = Join-Path $script:Game 'tf3snap'
    $hasProxy = Test-Ours $proxy 'vulkan-1.dll'
    $hasPlugin = Test-Ours $plugin 'tf3snap\tf3snap.dll'
    if (-not $hasProxy -and -not $hasPlugin -and -not $rec) { Say (L 'nothing'); return }
    if ((Test-Path -LiteralPath $proxy) -and -not $hasProxy) { Say (L 'notOurs' $proxy) 'Yellow' }

    $wipe = [bool]$RemoveData
    if (-not $wipe -and $script:Wizard) { $wipe = (Ask (L 'askRemoveData')) -match '^[yY]' }
    Assert-GameStopped

    if ($hasProxy) { Remove-Item -LiteralPath $proxy -Force; Say (L 'remove' $proxy) }
    $foreign = Prop $rec 'foreign'
    if ($foreign) {
        $b = Join-Path $script:Game $foreign.backup
        if (-not (Test-Path -LiteralPath $proxy) -and (Test-Path -LiteralPath $b) -and (Get-Sha $b) -eq $foreign.sha256) {
            Move-Item -LiteralPath $b -Destination $proxy
            Say (L 'restore' $proxy) 'Green'
        } elseif (Test-Path -LiteralPath $b) { Say (L 'restoreSkipped' $b) 'Yellow' }
    }
    if ($hasPlugin) { Remove-Item -LiteralPath $plugin -Force; Say (L 'remove' $plugin) }
    $recPath = Join-Path $dir 'install.json'
    if (Test-Path -LiteralPath $recPath) { Remove-Item -LiteralPath $recPath -Force }
    $backupDir = Join-Path $dir 'backup'
    $leftBackups = @(Get-ChildItem -LiteralPath $backupDir -File -ErrorAction SilentlyContinue)
    if ((Test-Path -LiteralPath $backupDir) -and -not $leftBackups.Count) { Remove-Item -LiteralPath $backupDir -Force }

    if ($wipe -and (Test-Path -LiteralPath $dir)) {
        # a backup that could not be put back is never deleted
        Get-ChildItem -LiteralPath $dir -Force | Where-Object { $_.Name -ne 'backup' } | Remove-Item -Recurse -Force
        if (-not $leftBackups.Count) { Remove-Item -LiteralPath $dir -Recurse -Force }
        Say (L 'removedAll') 'Green'
    } else {
        Say (L 'removed' $dir) 'Green'
    }
}

# ---- error report ---------------------------------------------------------------------------------------
function Read-Tail([string]$Path, [int]$MaxBytes = 65536) {
    $fs = [IO.File]::Open($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
    try {
        $start = [Math]::Max([int64]0, $fs.Length - $MaxBytes)
        [void]$fs.Seek($start, [IO.SeekOrigin]::Begin)
        $buf = New-Object byte[] ([int]($fs.Length - $start))
        $n = $fs.Read($buf, 0, $buf.Length)
        $text = [Text.Encoding]::UTF8.GetString($buf, 0, $n)
        if ($start -gt 0) { $text = "... (first $start bytes skipped)`r`n" + $text }
        return $text
    } finally { $fs.Dispose() }
}

function Find-GameDataDirs {
    $roots = @()
    if ($env:PUBLIC) {
        $roots += @(Get-ChildItem -LiteralPath (Join-Path $env:PUBLIC 'Documents\Steam') -Directory -ErrorAction SilentlyContinue |
                    ForEach-Object { $_.FullName })
    }
    $steam = Get-SteamPath
    if ($steam) {
        $roots += @(Get-ChildItem -LiteralPath (Join-Path $steam 'userdata') -Directory -ErrorAction SilentlyContinue |
                    ForEach-Object { $_.FullName })
    }
    return @($roots | ForEach-Object { Join-Path $_ "$AppId\local" } | Where-Object { Test-Path -LiteralPath $_ })
}

# Collects what a bug report needs into one UTF-8 text file.  Every section is fenced off, so a failing
# section only notes its error and the rest still gets written.  Never throws.
function Write-Report([string]$Reason, $Err) {
    $sb = New-Object Text.StringBuilder
    $section = {
        param([string]$Title, [scriptblock]$Body)
        [void]$sb.AppendLine()
        [void]$sb.AppendLine("==== $Title ====")
        try { foreach ($line in @(& $Body)) { [void]$sb.AppendLine([string]$line) } }
        catch { [void]$sb.AppendLine("(section failed: $($_.Exception.Message))") }
    }
    $fileBlock = {
        param([string]$Path, [int]$Max = 262144)
        if (Test-Path -LiteralPath $Path -PathType Leaf) { "--- $Path"; Read-Tail $Path $Max } else { "--- $Path (missing)" }
    }
    $dllLine = {
        param([string]$Path, [string]$Rel)
        if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return "$Rel : missing" }
        $who = if (Test-Ours $Path $Rel) { "ours, $(Get-Version $Path)" } else { 'NOT ours' }
        "$Rel : $who; $(Describe-File $Path); sha256 $(Get-Sha $Path)"
    }

    & $section "$Product error report" {
        "time      : $((Get-Date).ToString('yyyy-MM-dd HH:mm:ss zzz'))"
        "reason    : $Reason"
        "action    : $script:ActionName"
        "language  : $Lang (UI culture $ui)"
        "package   : $script:Source"
        "game      : $script:Game"
    }
    if ($Err) {
        & $section 'Error' {
            "$($Err.Exception.GetType().FullName): $($Err.Exception.Message)"
            if ($Err.Exception.InnerException) { "inner: $($Err.Exception.InnerException.Message)" }
            if ($Err.InvocationInfo) { $Err.InvocationInfo.PositionMessage }
            $Err.ScriptStackTrace
        }
    }
    & $section 'Installer output' { $script:Log }
    & $section 'System' {
        try {
            $os = Get-CimInstance -ClassName Win32_OperatingSystem -ErrorAction Stop
            "OS        : $($os.Caption) $($os.Version) build $($os.BuildNumber) $($os.OSArchitecture)"
        } catch { "OS        : $([Environment]::OSVersion.VersionString)" }
        "PowerShell: $($PSVersionTable.PSVersion) ($([IntPtr]::Size * 8)-bit)"
        $admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
            [Security.Principal.WindowsBuiltInRole]::Administrator)
        "Admin     : $admin"
        "Steam     : $(Get-SteamPath)"
    }
    & $section 'Package files' {
        if ($script:Source) {
            $plugin = Join-Path $script:Source 'tf3snap\tf3snap.dll'
            if (-not (Test-Path -LiteralPath $plugin)) { $plugin = Join-Path $script:Source 'tf3snap.dll' }
            & $dllLine (Join-Path $script:Source 'vulkan-1.dll') 'vulkan-1.dll'
            & $dllLine $plugin 'tf3snap\tf3snap.dll'
        } else { '(unknown)' }
    }
    if ($script:Game) {
        $g = $script:Game
        & $section 'Game folder' {
            "folder    : $g"
            $exe = Join-Path $g $GameExe
            if (Test-Path -LiteralPath $exe) { "$GameExe : $(Describe-File $exe); sha256 $(Get-Sha $exe)" } else { "$GameExe : missing" }
            $running = @(Get-Process -Name ([IO.Path]::GetFileNameWithoutExtension($GameExe)) -ErrorAction SilentlyContinue)
            "running   : $($running.Count -gt 0)"
            & $dllLine (Join-Path $g 'vulkan-1.dll') 'vulkan-1.dll'
            & $dllLine (Join-Path $g 'tf3snap\tf3snap.dll') 'tf3snap\tf3snap.dll'
            "disabled marker: $(Test-Path -LiteralPath (Join-Path $g 'tf3snap\disabled'))"
        }
        & $section 'tf3snap folder' {
            Get-ChildItem -LiteralPath (Join-Path $g 'tf3snap') -Recurse -Force -ErrorAction SilentlyContinue |
                ForEach-Object { '{0,-48} {1,10:N0}  {2:yyyy-MM-dd HH:mm:ss}' -f $_.FullName.Substring($g.Length + 1),
                    $(if ($_.PSIsContainer) { 0 } else { $_.Length }), $_.LastWriteTime }
        }
        # other loaders / proxies in the game root (ReShade, other mods ...) are a common cause of trouble
        & $section 'DLLs in the game folder' {
            Get-ChildItem -LiteralPath $g -Filter '*.dll' -File -ErrorAction SilentlyContinue |
                ForEach-Object { '{0,-34} {1}' -f $_.Name, (Describe-File $_.FullName) }
        }
        foreach ($f in 'install.json', 'tf3snap.ini', 'proxy_error.txt', 'tf3snap.log') {
            & $section "tf3snap\$f" { & $fileBlock (Join-Path $g "tf3snap\$f") }
        }
        # the plugin keeps the 4 sessions before the current one in logs\ (newest first by name);
        # tf3snap.prev.log is where 0.2.1 and older kept the previous one
        $older = @(Get-ChildItem -LiteralPath (Join-Path $g 'tf3snap\logs') -Filter 'tf3snap_*.log' -File -ErrorAction SilentlyContinue |
                   Sort-Object Name -Descending | Select-Object -First 4)
        $older += @(Get-Item -LiteralPath (Join-Path $g 'tf3snap\tf3snap.prev.log') -ErrorAction SilentlyContinue)
        if (-not $older.Count) { & $section 'tf3snap\logs' { '(no earlier session logs)' } }
        foreach ($o in $older) {
            & $section ('tf3snap\' + $o.FullName.Substring((Join-Path $g 'tf3snap').Length + 1)) { & $fileBlock $o.FullName }
        }
    }
    foreach ($local in @(Find-GameDataDirs)) {
        $cd = Join-Path $local 'crash_dump'
        & $section "Game log: $cd" {
            & $fileBlock (Join-Path $cd 'error.json') 32768
            & $fileBlock (Join-Path $cd 'stdout.txt') 98304
        }
        & $section "Recent crash reports: $cd" {
            $files = @(Get-ChildItem -LiteralPath $cd -File -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending)
            '-- newest dumps:'
            $files | Where-Object { $_.Extension -eq '.dmp' } | Select-Object -First 5 |
                ForEach-Object { '{0}  {1:N0} bytes  {2:yyyy-MM-dd HH:mm:ss}' -f $_.Name, $_.Length, $_.LastWriteTime }
            $files | Where-Object { $_.Extension -eq '.json' -and $_.Name -ne 'error.json' } | Select-Object -First 3 |
                ForEach-Object { & $fileBlock $_.FullName 16384 }
            $files | Where-Object { $_.Extension -eq '.txt' -and $_.Name -ne 'stdout.txt' } | Select-Object -First 2 |
                ForEach-Object { & $fileBlock $_.FullName 16384 }
        }
    }
    & $section 'Windows crash events for the game (14 days)' {
        $ev = @(Get-WinEvent -FilterHashtable @{ LogName = 'Application'; Id = 1000, 1002; StartTime = (Get-Date).AddDays(-14) } `
                    -MaxEvents 300 -ErrorAction SilentlyContinue | Where-Object { $_.Message -match 'TransportFever3' } |
                Select-Object -First 8)
        if (-not $ev.Count) { '(none)' }
        foreach ($e in $ev) {
            "--- $($e.TimeCreated.ToString('yyyy-MM-dd HH:mm:ss'))  event $($e.Id)"
            ($e.Message -split "`r?`n" | Select-Object -First 14) -join "`r`n"
        }
    }

    $text = $sb.ToString()
    $utf8 = New-Object Text.UTF8Encoding $true
    foreach ($dir in @($script:ReportDir, $env:TEMP)) {
        if (-not $dir) { continue }
        $path = Join-Path $dir $ReportName
        try {
            [IO.File]::WriteAllText($path, $text, $utf8)
            Say (L 'reportWritten' $path) 'Cyan'
            Say (L 'reportHint')
            if ($script:Wizard) { Start-Process -FilePath 'explorer.exe' -ArgumentList "/select,`"$path`"" -ErrorAction SilentlyContinue }
            return
        } catch { Say (L 'reportFailed' $path) 'Red' }
    }
}

# ---- main ------------------------------------------------------------------------------------------------
$script:Wizard = -not $GameDir -and -not $Action
$script:Source = $null
$script:Game = $null
$script:ActionName = $Action
$script:ReportDir = Split-Path -Path $PSScriptRoot -Parent   # where the .bat is
try {
    if (-not $SourceRoot) { $SourceRoot = $script:ReportDir }
    $script:Source = (Resolve-Path -LiteralPath $SourceRoot).ProviderPath.TrimEnd('\')
    $found = if ($GameDir) { $null } else { Find-GameDir $script:Source }
    $known = if ($GameDir) { $GameDir.Trim().Trim('"') } else { $found }
    try { $pl = Get-PanelLang $known } catch { $pl = $null }
    if ($pl) { $Lang = $pl }
    $pkgPlugin = Join-Path $script:Source 'tf3snap\tf3snap.dll'
    if (-not (Test-Path -LiteralPath $pkgPlugin)) { $pkgPlugin = Join-Path $script:Source 'tf3snap.dll' }
    $title = if (Test-Path -LiteralPath $pkgPlugin) { Get-Version $pkgPlugin } else { '' }
    Say (L 'title' $title) 'Cyan'
    Say ''

    # The wizard asks again after a typo or a wrong folder (3 tries); only then is it a failure with a report.
    if (-not $Action) {
        Say (L 'menu1'); Say (L 'menu2'); Say (L 'menu3')
        for ($try = 1; -not $Action; ++$try) {
            $c = Ask (L 'choice')
            switch ($c) {
                '' { $Action = 'Install' } '1' { $Action = 'Install' } '2' { $Action = 'Uninstall' } '3' { $Action = 'Report' }
                default { if ($try -ge 3) { throw (L 'badChoice' $c) }; Say (L 'badChoice' $c) 'Yellow' }
            }
        }
    }
    if ($Action -notin 'Install', 'Uninstall', 'Report') { throw (L 'badChoice' $Action) }
    $script:ActionName = $Action

    # A report is still useful without a game folder, so for Report a missing / wrong folder is not an error.
    if ($GameDir) {
        try { $script:Game = Resolve-GameDir $GameDir }
        catch { if ($Action -ne 'Report') { throw }; Say $_.Exception.Message 'Yellow' }
    } elseif ($found -and -not $script:Wizard) {
        Say (L 'found' $found) 'Cyan'   # unattended (-Action given): take it
        $script:Game = $found
    } elseif ($script:Wizard) {
        if ($found) { Say (L 'found' $found) 'Cyan' }
        for ($try = 1; -not $script:Game; ++$try) {
            $in = Ask $(if ($found) { L 'askDirDefault' } else { L 'askDir' })
            $cand = if ($in.Trim('"')) { $in } elseif ($found) { $found } else { '' }
            try { $script:Game = Resolve-GameDir $cand }
            catch {
                if ($Action -eq 'Report' -and (-not $cand -or $try -ge 3)) { break }
                if ($try -ge 3) { throw }
                Say $_.Exception.Message 'Yellow'
            }
        }
    } elseif ($Action -ne 'Report') {
        throw (L 'noDir')
    }
    if ($Action -eq 'Report') {
        Write-Report 'requested by the user' $null
        exit 0
    }
    Assert-GameStopped
    if ($Action -eq 'Install') { Invoke-Install } else { Invoke-Uninstall }
    exit 0
} catch {
    $err = $_
    Say (L 'failed' $err.Exception.Message) 'Red'
    $denied = $err.Exception -is [UnauthorizedAccessException] -or
              ($err.Exception.InnerException -is [UnauthorizedAccessException]) -or
              $err.FullyQualifiedErrorId -match 'UnauthorizedAccess'
    if ($denied) { Say (L 'admin') 'Yellow' }
    Write-Report 'installer failed' $err
    exit 1
}
