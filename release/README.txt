Better Snap Option（更好的吸附貼齊）— Transport Fever 3 snapping panel
(0.2.x was called "TF3 Snap Options"; the installer updates it in place.)
繁體中文 / 简体中文 / English / 日本語 (below)

======================================================================
繁體中文
======================================================================
在道路 / 鐵路工具旁加一個小面板，可以分別開關各種貼齊與引導線。

安裝 / 更新
  1. 完全關閉遊戲。把整個壓縮檔解壓到任意資料夾。
  2. 雙擊「安裝-Install.bat」，選 1，確認遊戲資料夾。Steam 版會自動找到；找不到時，
     把含 TransportFever3.exe 的資料夾拖進視窗再按 Enter。
  3. 啟動遊戲，打開道路或鐵路工具就會出現面板。面板最下面可以切換語言。
  更新時用新版的安裝檔再執行一次即可，設定與面板文字都會保留。

移除：執行同一個安裝檔，選 2。如果原本遊戲資料夾裡有別的 vulkan-1.dll（其他模組的），會自動還原。
暫時停用：在遊戲資料夾的 tf3snap 資料夾裡建立一個名為 disabled 的空檔案。

遇到問題
  安裝失敗時會自動在安裝檔旁邊產生「報錯.log」。遊戲裡看不到面板或遊戲崩潰時，執行安裝檔選 3 產生。
  回報問題時請附上這個檔案。檔案裡有電腦上的資料夾路徑（可能含使用者名稱），公開貼出前可以先看一下。

運作方式：本外掛是一個 vulkan-1.dll 轉接器，所有 Vulkan 呼叫都轉給系統原本的 vulkan-1.dll，
只額外載入 tf3snap\tf3snap.dll。不修改任何遊戲檔案。遊戲更新後若對不上，外掛會自動停用貼齊功能
（面板顯示紅字），不影響遊戲本身。面板文字可以在 tf3snap\tf3snap_text_*.ini 自行修改。

======================================================================
简体中文
======================================================================
在道路 / 铁路工具旁增加一个小面板，可以分别开关各种吸附与辅助线。

安装 / 更新
  1. 完全退出游戏。把整个压缩包解压到任意文件夹。
  2. 双击“安裝-Install.bat”，选 1，确认游戏目录。Steam 版会自动找到；找不到时，
     把含 TransportFever3.exe 的文件夹拖进窗口再按 Enter。
  3. 启动游戏，打开道路或铁路工具就会出现面板。面板最下方可以切换语言。
  更新时用新版的安装文件再运行一次即可，设置与面板文字都会保留。

卸载：运行同一个安装文件，选 2。如果原本游戏目录里有其他 vulkan-1.dll（其他模组的），会自动还原。
暂时停用：在游戏目录的 tf3snap 文件夹里新建一个名为 disabled 的空文件。

遇到问题
  安装失败时会自动在安装文件旁生成“報錯.log”。游戏中看不到面板或游戏崩溃时，运行安装文件选 3 生成。
  反馈问题时请附上此文件。文件中含有电脑上的文件夹路径（可能包含用户名），公开发布前可以先检查一下。

工作方式：本插件是一个 vulkan-1.dll 转接器，所有 Vulkan 调用都转给系统原本的 vulkan-1.dll，
只额外加载 tf3snap\tf3snap.dll。不修改任何游戏文件。游戏更新后若不匹配，插件会自动停用吸附功能
（面板显示红字），不影响游戏本身。面板文字可以在 tf3snap\tf3snap_text_*.ini 中自行修改。

======================================================================
English
======================================================================
Adds a small panel next to the road / track tool that switches each kind of snapping and guide line
on or off.

Install / update
  1. Close the game completely. Extract the whole zip to any folder.
  2. Double-click "安裝-Install.bat", choose 1 and confirm the game folder. Steam installs are found
     automatically; otherwise drag the folder that contains TransportFever3.exe into the window and press Enter.
  3. Start the game and open the road or track tool: the panel appears. Its language can be changed at the
     bottom of the panel.
  To update, run the installer of the new version; settings and panel texts are kept.

Uninstall: run the same installer and choose 2. If the game folder had another mod's vulkan-1.dll before,
it is put back.
Disable for a while: create an empty file named "disabled" in the game's tf3snap folder.

Problems
  A failed installation writes "報錯.log" (error report) next to the installer automatically. If the panel
  does not show up in the game or the game crashes, run the installer and choose 3 to write one.
  Attach that file when you report the problem. It contains folder paths from your PC (possibly your user
  name); check it before posting it publicly.

How it works: the plugin is a vulkan-1.dll shim that hands every Vulkan call to the system's own
vulkan-1.dll and additionally loads tf3snap\tf3snap.dll. No game file is modified. If a game update no
longer matches, the plugin switches its snapping features off by itself (red text in the panel) and the
game runs normally. The panel texts can be edited in tf3snap\tf3snap_text_*.ini.

======================================================================
日本語
======================================================================
道路 / 線路ツールの横に小さなパネルを追加し、各種スナップとガイドを個別にオン / オフできます。

インストール / 更新
  1. ゲームを完全に終了し、zip 全体を任意のフォルダーに展開します。
  2. 「安裝-Install.bat」をダブルクリックし、1 を選んでゲームフォルダーを確認します。Steam 版は自動で
     見つかります。見つからない場合は TransportFever3.exe があるフォルダーをウィンドウにドラッグして
     Enter を押してください。
  3. ゲームを起動して道路または線路ツールを開くとパネルが表示されます。言語はパネル下部で切り替えられます。
  更新するときは新しいバージョンのインストーラーをもう一度実行します。設定とパネルの文字は保持されます。

アンインストール：同じインストーラーを実行して 2 を選びます。ゲームフォルダーに元々別の MOD の
vulkan-1.dll があった場合は自動で元に戻します。
一時的に無効化：ゲームフォルダーの tf3snap フォルダーに disabled という名前の空ファイルを作成します。

問題が起きたとき
  インストールに失敗すると、インストーラーの横に「報錯.log」（エラーレポート）が自動で作成されます。
  ゲーム内にパネルが出ない・クラッシュする場合は、インストーラーを実行して 3 を選ぶと作成できます。
  問題を報告するときはこのファイルを添付してください。PC のフォルダーパス（ユーザー名を含む場合が
  あります）が含まれているので、公開する前に確認してください。

仕組み：本プラグインは vulkan-1.dll の中継 DLL で、すべての Vulkan 呼び出しをシステム本来の
vulkan-1.dll に渡し、追加で tf3snap\tf3snap.dll を読み込むだけです。ゲームのファイルは一切変更しません。
ゲームの更新で合わなくなった場合、プラグインはスナップ機能を自動で無効にし（パネルに赤字で表示）、
ゲーム自体には影響しません。パネルの文字は tf3snap\tf3snap_text_*.ini で編集できます。

----------------------------------------------------------------------
License: MIT (LICENSE), Copyright (c) 2026 Hsuamber
Third-party licenses: licenses\
Source: https://github.com/Hsuamber/Transport-Fever-3-Better-Snap-Option-mod
