# 更好的吸附貼齊 Better Snap Option

[繁體中文](#繁體中文) · [简体中文](#简体中文) · [English](#english) · [日本語](#日本語)

---

## 繁體中文

《Transport Fever 3》的道路輔助建造貼齊工具，由於TF3的自動貼齊工具太爛了，因此我請Claude幫做了一個小工具，靈感來自《Cities: Skylines II》：點擊道路 / 鐵路工具時顯示一個面板，可分別開關各種貼齊與引導線、調整貼齊範圍、設定角度格線。

> 非官方工具，與 Urban Games、Paradox Interactive 無關。本工具只在記憶體內運作，不修改任何遊戲檔案；移除即完全恢復。

### 下載與安裝

1. 到 [Releases](../../releases) 下載最新的 `Better-Snap-Option-v<版本>.zip`，完全關閉遊戲，解壓到任意資料夾。
2. 雙擊 `安裝-Install.bat` 輸入 1，確認遊戲資料夾（Steam 版會自動找到；找不到就把含 `TransportFever3.exe` 的資料夾拖進視窗）。
3. 打開遊戲實際進入遊戲裡面，只要打開道路或鐵路工具就會出現面板；面板最下面可以切換語言。

更新：用新版的安裝檔再執行一次，設定與面板文字都會保留。移除：同一個安裝檔選 2。
遇到問題：安裝檔選 3 會產生 `報錯.log`，回報時附上（裡面有電腦上的資料夾路徑，公開貼出前可以先看一下）。
發布包裡的 `README.txt` 是給玩家的四語說明。

### 功能

面板只在**道路 / 鐵路工具開著**時出現；
面板分兩塊：**貼齊**管游標會不會被吸過去，**引導線**管畫面上有哪些引導線。

| 貼齊 | 作用 |
|---|---|
| 1 貼齊既有幾何 | 吸附既有道路 / 軌道 |
| 2 貼齊長度刻度 | 吸取新拉路段長度 N 公尺的倍數 |
| 3 貼齊道路側邊（並排） | 吸附道路側邊 |
| 4 貼齊車道（重疊） | 吸附既有道路的車道位置 |
| 5 貼齊引導線 | 吸附引導線 |
| 6 貼齊鄰近路口 | 關閉後可在既有路口旁更近處接新節點 |
| 貼齊範圍 | 貼齊半徑與候選範圍倍率（0.25×–4×） |

| 引導線 | 作用 |
|---|---|
| 顯示引導線 | 是否顯示引導線 |
| 角度導引線 | 顯示±45° / ±90° 那類斜線；格線可選遊戲預設 / 90° / 45° / 30° / 15° |
| 縱向導引線 | 顯示既有道路方向延伸的線 |
| 橫向導引線 | 顯示附近幾條並排道路的端點對齊線；顯示為藍色虛線 |

取消某一種引導線則不顯示也不吸附。

### 風險（請先讀）

- 本工具為非官方工具，與 Urban Games、Paradox Interactive 皆無任何關係。使用可能產生崩潰或資料損壞等風險，請自負風險；不保證相容性、穩定性或帳號安全。
- 這是把自己的 DLL 讓遊戲載入、並在記憶體裡修改遊戲函式的工具。**防毒軟體可能誤報**；遊戲或顯卡驅動更新後可能需要重新產生特徵碼或暫時停用。

### 安裝

`安裝-Install.bat`：雙擊`安裝-Install.bat`之後 →輸入 1 安裝或更新 / 2 移除 / 3 產生報錯檔。

- 遊戲資料夾自動找：安裝包上層、Steam 註冊表 + `libraryfolders.vdf` 裡的每個遊戲庫；找不到就請把遊戲資料夾拖進視窗。選錯選項或資料夾會重問。

手動安裝也可以：`vulkan-1.dll` 放遊戲資料夾（與 `TransportFever3.exe` 同一層），然後把 `tf3snap\`資料夾放進去遊戲主資料夾裡。

### 授權

本工具原始碼依 [MIT 授權](LICENSE) 釋出，Copyright (c) 2026 Hsuamber。相依的 Dear ImGui（MIT）、MinHook（BSD 2-Clause）見 `release\licenses\THIRD-PARTY.txt`；它們的授權原文由 `package.bat` 從 `toolchain\` 複製進發布包的 `licenses\`。

[↑ 回到頂端](#更好的吸附貼齊-better-snap-option)

---

## 简体中文

《Transport Fever 3》的道路建造辅助吸附工具。TF3 自带的自动吸附实在太难用了，所以我请 Claude 帮忙做了这个小工具，灵感来自《Cities: Skylines II》：打开道路 / 铁路工具时会显示一个面板，可以分别开关各种吸附与辅助线、调整吸附范围、设置角度网格。

> 非官方工具，与 Urban Games、Paradox Interactive 无关。本工具只在内存中运行，不修改任何游戏文件；卸载即完全恢复。

### 下载与安装

1. 到 [Releases](../../releases) 下载最新的 `Better-Snap-Option-v<版本>.zip`，完全退出游戏，解压到任意文件夹。
2. 双击 `安裝-Install.bat`，输入 1，确认游戏目录（Steam 版会自动找到；找不到时把含 `TransportFever3.exe` 的文件夹拖进窗口）。
3. 启动游戏并真正进入游戏（不是停在主菜单），打开道路或铁路工具就会出现面板；面板最下方可以切换语言。

更新：用新版的安装文件再运行一次，设置与面板文字都会保留。卸载：用同一个安装文件选 2。
遇到问题：安装文件选 3 会生成 `報錯.log`，反馈时请附上（里面有电脑上的文件夹路径，公开发布前可以先检查一下）。
发布包里的 `README.txt` 是给玩家的四语说明。

### 功能

面板只在**道路 / 铁路工具打开**时出现；
面板分两块：**吸附**决定光标会不会被吸过去，**辅助线**决定画面上显示哪些辅助线。

| 吸附 | 作用 |
|---|---|
| 1 吸附到已有道路 / 轨道 | 吸附已有道路 / 轨道 |
| 2 按长度吸附 | 新拉路段的长度取 N 米的倍数 |
| 3 吸附道路边缘（并排） | 吸附道路侧边 |
| 4 吸附车道（重叠） | 吸附已有道路的车道位置 |
| 5 吸附辅助线 | 吸附辅助线 |
| 6 吸附附近路口节点 | 关闭后可以在已有路口旁更近的位置连接新节点 |
| 吸附范围 | 吸附半径与候选范围的倍率（0.25×–4×） |

| 辅助线 | 作用 |
|---|---|
| 显示辅助线 | 是否显示辅助线 |
| 角度辅助线 | 显示 ±45° / ±90° 这类斜线；网格可选游戏默认 / 90° / 45° / 30° / 15° |
| 纵向辅助线 | 显示沿已有道路方向延伸的线 |
| 横向辅助线 | 显示附近几条并排道路的端点对齐线；显示为蓝色虚线 |

关闭某一种辅助线后，它既不显示也不吸附。

### 风险（请先阅读）

- 本工具为非官方工具，与 Urban Games、Paradox Interactive 均无任何关系。使用可能导致崩溃或数据损坏等风险，风险自负；不保证兼容性、稳定性或账号安全。
- 本工具会让游戏加载自己的 DLL，并在内存中修改游戏函数。**杀毒软件可能误报**；游戏或显卡驱动更新后可能需要重新生成特征码或暂时停用。

### 安装

`安裝-Install.bat`：双击后 → 输入 1 安装或更新 / 2 卸载 / 3 生成报错文件。

- 自动查找游戏目录：安装包的上一层、Steam 注册表 + `libraryfolders.vdf` 中的每个游戏库；找不到就请把游戏目录拖进窗口。选错选项或文件夹会重新询问。

也可以手动安装：`vulkan-1.dll` 放在游戏目录（与 `TransportFever3.exe` 同一层），然后把 `tf3snap\` 文件夹放进游戏主目录里。

### 授权

本工具源代码依 [MIT 许可证](LICENSE) 发布，Copyright (c) 2026 Hsuamber。依赖的 Dear ImGui（MIT）、MinHook（BSD 2-Clause）见 `release\licenses\THIRD-PARTY.txt`；它们的许可证原文由 `package.bat` 从 `toolchain\` 复制到发布包的 `licenses\`。

[↑ 返回顶部](#更好的吸附貼齊-better-snap-option)

---

## English

A snapping helper for building roads in Transport Fever 3. TF3's built-in auto-snapping is really bad, so I asked Claude
to make a small tool, inspired by Cities: Skylines II: when you open the road / track tool, a panel appears where you can
switch each kind of snapping and guide line on or off, adjust the snap range and set the angle grid.

> Unofficial tool, not affiliated with Urban Games or Paradox Interactive. It works only in memory and does not modify any game
> file; uninstalling it restores everything.

### Download and install

1. Download the latest `Better-Snap-Option-v<version>.zip` from [Releases](../../releases), close the game completely
   and extract the zip to any folder.
2. Double-click `安裝-Install.bat`, enter 1 and confirm the game folder (Steam installs are found automatically;
   otherwise drag the folder that contains `TransportFever3.exe` into the window).
3. Start the game and get into an actual game (not just the main menu); the panel appears as soon as you open the road
   or track tool. The language can be changed at the bottom of the panel.

Update: run the installer of the new version again; settings and panel texts are kept. Uninstall: run the same
installer and choose 2.
Problems: choose 3 in the installer to write `報錯.log` (error report) and attach it to your report (it contains folder
paths from your PC; check it before posting it publicly).
The `README.txt` in the download is the player guide in all four languages.

### Features

The panel only shows while the **road / track tool is open**.
It has two parts: **Snapping** decides whether the cursor gets pulled to something, **Guides** decides which guide lines
are shown.

| Snapping | What it does |
|---|---|
| 1 Snap to existing roads / tracks | Snaps to existing roads / tracks |
| 2 Snap length | Makes the length of the new segment a multiple of N metres |
| 3 Snap to road edges (side by side) | Snaps to the side of a road |
| 4 Snap to lanes (overlapping) | Snaps to the lane positions of existing roads |
| 5 Snap to guides | Snaps to guide lines |
| 6 Snap to nearby junction nodes | Turn it off to connect a new node closer to an existing junction |
| Snap range | Multiplier for the snap radius and the candidate range (0.25×–4×) |

| Guides | What it does |
|---|---|
| Show guides | Whether guide lines are shown |
| Angle guides | Diagonal lines such as ±45° / ±90°; grid: game default / 90° / 45° / 30° / 15° |
| Straight guides | Lines extending along existing roads |
| Cross guides | Lines that line up the ends of several nearby parallel roads; shown as blue dashed lines |

A guide type that is switched off is neither shown nor snapped to.

### Risks (read first)

- This is an unofficial tool, not affiliated with Urban Games or Paradox Interactive. Using it may cause crashes, data
  corruption or other problems; use it at your own risk. Compatibility, stability and account safety are not guaranteed.
- The tool makes the game load its own DLL and modifies game functions in memory. **Antivirus software may report it
  (false positive)**; after a game or graphics driver update the signatures may need to be regenerated, or the tool
  disabled for a while.

### Install

`安裝-Install.bat`: double-click it, then enter 1 to install or update / 2 to uninstall / 3 to write an error report.

- Finding the game folder: the folder above the package and every library from the Steam registry +
  `libraryfolders.vdf`; if none is found, drag the game folder into the window. A wrong menu choice or folder is
  asked again.

Manual install works too: put `vulkan-1.dll` in the game folder (next to `TransportFever3.exe`), then put the
`tf3snap\` folder into the game's main folder.

### License

The source code is released under the [MIT License](LICENSE), Copyright (c) 2026 Hsuamber. The dependencies Dear ImGui
(MIT) and MinHook (BSD 2-Clause) are listed in `release\licenses\THIRD-PARTY.txt`; `package.bat` copies their license
texts from `toolchain\` into the package's `licenses\`.

[↑ Back to top](#更好的吸附貼齊-better-snap-option)

---

## 日本語

Transport Fever 3 の道路建設を補助するスナップツールです。TF3 標準の自動スナップがあまりにも使いにくいので、Claude に頼んで小さなツールを作ってもらいました。Cities: Skylines II を参考にしており、道路 / 線路ツールを開くとパネルが表示され、各種スナップとガイドの個別オン / オフ、スナップ範囲の調整、角度グリッドの設定ができます。

> 非公式ツールであり、Urban Games、Paradox Interactive とは関係ありません。メモリ上でのみ動作し、ゲームのファイルは一切変更しません。アンインストールすれば完全に元に戻ります。

### ダウンロードとインストール

1. [Releases](../../releases) から最新の `Better-Snap-Option-v<バージョン>.zip` をダウンロードし、ゲームを完全に終了してから任意のフォルダーに展開します。
2. `安裝-Install.bat` をダブルクリックし、1 を入力してゲームフォルダーを確認します（Steam 版は自動で見つかります。見つからない場合は `TransportFever3.exe` があるフォルダーをウィンドウにドラッグしてください）。
3. ゲームを起動して実際のプレイ画面に入り（メインメニューではなく）、道路または線路ツールを開くとパネルが表示されます。言語はパネル下部で切り替えられます。

更新：新しいバージョンのインストーラーをもう一度実行します。設定とパネルの文字は保持されます。アンインストール：同じインストーラーで 2 を選びます。
問題が起きたとき：インストーラーで 3 を選ぶと `報錯.log`（エラーレポート）が作成されるので、報告時に添付してください（PC のフォルダーパスが含まれているので、公開する前に確認してください）。
配布パッケージの `README.txt` は、プレイヤー向けの 4 言語の説明書です。

### 機能

パネルは**道路 / 線路ツールを開いている間**だけ表示されます。
パネルは 2 つに分かれており、**スナップ**はカーソルが吸い寄せられるかどうか、**ガイド**は画面にどのガイド線を表示するかを決めます。

| スナップ | 説明 |
|---|---|
| 1 既存の道路 / 線路にスナップ | 既存の道路 / 線路に吸着します |
| 2 長さにスナップ | 新しく引く区間の長さを N m の倍数にします |
| 3 道路の側端にスナップ（並列） | 道路の側端に吸着します |
| 4 車線にスナップ（重なり） | 既存道路の車線位置に吸着します |
| 5 ガイドにスナップ | ガイド線に吸着します |
| 6 近くの交差点ノードにスナップ | オフにすると、既存の交差点のすぐ近くに新しいノードをつなげられます |
| スナップ範囲 | スナップ半径と候補範囲の倍率（0.25×–4×） |

| ガイド | 説明 |
|---|---|
| ガイドを表示 | ガイド線を表示するかどうか |
| 角度ガイド | ±45° / ±90° などの斜線を表示します。グリッドはゲーム既定 / 90° / 45° / 30° / 15° から選べます |
| 縦方向ガイド | 既存道路の方向に延びる線を表示します |
| 横方向ガイド | 近くにある数本の並列道路の端点を揃える線を表示します（青い破線） |

オフにした種類のガイドは、表示もスナップもされません。

### リスク（最初にお読みください）

- 本ツールは非公式ツールであり、Urban Games、Paradox Interactive とは一切関係ありません。使用によりクラッシュやデータ破損などが起きる可能性があります。自己責任でご利用ください。互換性・安定性・アカウントの安全は保証しません。
- 本ツールは独自の DLL をゲームに読み込ませ、メモリ上でゲームの関数を書き換えます。**ウイルス対策ソフトが誤検知する可能性があります**。ゲームやグラフィックドライバーの更新後は、シグネチャの再生成や一時的な無効化が必要になることがあります。

### インストール

`安裝-Install.bat`：ダブルクリックして、1（インストール / 更新）、2（アンインストール）、3（エラーレポートを作成）のいずれかを入力します。

- ゲームフォルダーの自動検出：パッケージの 1 つ上のフォルダー、Steam のレジストリと `libraryfolders.vdf` にある各ライブラリ。見つからない場合は、ゲームフォルダーをウィンドウにドラッグしてください。メニューやフォルダーの選択を誤ると聞き直します。

手動でもインストールできます：`vulkan-1.dll` をゲームフォルダー（`TransportFever3.exe` と同じ階層）に置き、`tf3snap\` フォルダーをゲームのメインフォルダーに入れます。

### ライセンス

本ツールのソースコードは [MIT ライセンス](LICENSE) で公開しています。Copyright (c) 2026 Hsuamber。依存している Dear ImGui（MIT）と MinHook（BSD 2-Clause）については `release\licenses\THIRD-PARTY.txt` を参照してください。それらのライセンス原文は `package.bat` が `toolchain\` から配布パッケージの `licenses\` にコピーします。

[↑ ページ先頭へ](#更好的吸附貼齊-better-snap-option)
