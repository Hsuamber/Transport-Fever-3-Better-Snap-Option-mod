# 更好的吸附貼齊 Better Snap Option

《Transport Fever 3》的道路輔助建造貼齊工具，由於TF3的自動貼齊工具太爛了，因此我請Claude幫做了一個小工具，靈感來自《Cities: Skylines II》：點擊道路 / 鐵路工具時顯示一個面板，
可分別開關各種貼齊與引導線、調整貼齊範圍、設定角度格線。

*A snapping-options panel for Transport Fever 3: switch each kind of road / track snapping and guide line on or off,
like Cities: Skylines II. The panel is available in 繁體中文 / 简体中文 / English / 日本語; the download includes an
install guide in all four (`README.txt`).*

**目前版本：0.3.1**。修正附近道路很密集時，打開道路工具會崩潰（Internal error）的問題。

[下載安裝包](https://github.com/Hsuamber/Transport-Fever-3-Better-Snap-Option-mod/releases/latest) · [問題回報](https://github.com/Hsuamber/Transport-Fever-3-Better-Snap-Option-mod/issues)

> 非官方工具，與 Urban Games / Good Shepherd 無關。本工具只在記憶體內運作，不修改任何遊戲檔案；移除即完全恢復。

## 安裝、更新與移除

需要 Windows x64，僅供單機遊玩。

1. 到 [Releases](https://github.com/Hsuamber/Transport-Fever-3-Better-Snap-Option-mod/releases/latest) 下載 **Better-Snap-Option-v0.3.1.zip**。
2. 完全關閉遊戲，把整個壓縮檔解壓到任意資料夾。
3. 雙擊 **安裝-Install.bat**，選 **1**，確認遊戲資料夾（Steam 版會自動找到；找不到就把含 `TransportFever3.exe` 的資料夾拖進視窗再按 Enter）。
4. 打開遊戲實際進入遊戲里面，只要打開道路或鐵路工具就會出現面板；面板最下面可以切換語言。

更新：不用先移除，用新版的安裝檔再執行一次、選 **1**，設定與面板文字都會保留。

移除：完全關閉遊戲，執行同一個安裝檔選 **2**，最後會問要不要連設定一起刪（預設保留）。
遊戲資料夾原本若已有其他模組的 `vulkan-1.dll`，安裝時會先詢問並備份，移除時自動還原。

暫時停用：在遊戲資料夾的 `tf3snap` 資料夾裡建立一個名為 `disabled` 的空檔案；刪掉它就恢復。

## 開始使用

面板分兩塊：**貼齊**管游標會不會被吸過去，**引導線**管畫面上有哪些線。

| 想做什麼 | 操作 |
|---|---|
| 打開面板 | 進入遊戲後打開道路或鐵路工具；關掉工具約半秒後自動收起 |
| 移動面板 | 拖曳最上面的標題列，位置會記住 |
| 切換語言 | 面板最下面的語言選單：繁體中文 / 简体中文 / English / 日本語 |
| 面板一直顯示 | 取消勾選最下面的「只在道路 / 鐵路工具開啟時顯示」 |
| 自由放置、不被吸過去 | 關閉 **1 貼齊既有道路 / 軌道**，需要時再關閉 **5 貼齊引導線** |
| 新路段長度取整數 | 開啟 **2 貼齊長度**，在「每 N 公尺」輸入間隔，按 Enter |
| 並排道路不要緊貼在一起 | 關閉 **3 貼齊道路側邊** |
| 讓幾條並排道路的端點切齊 | 開啟 **橫向導引線**，拉路時終點靠近藍色虛線就會吸過去 |
| 斜線改成固定角度 | 在 **角度導引線** 旁選擇格線：遊戲預設 / 90° / 45° / 30° / 15° |
| 吸附太強或太弱 | 調整 **貼齊範圍**（0.25×–4×） |
| 回到遊戲原本的行為 | 按 **恢復遊戲預設** |

點面板不會搶走遊戲的鍵盤（Shift、ESC 等快捷鍵照常），只有在長度輸入框打字時暫時接管，按 Enter 就交回遊戲。
ESC 暫停選單開著、或切到別的程式時，面板一律隱藏。

### 各選項說明

| 貼齊 | 作用 |
|---|---|
| 1 貼齊既有道路 / 軌道 | 關閉後游標不吸附既有道路 / 軌道 |
| 2 貼齊長度 | 新路段長度取 N 公尺的倍數 |
| 3 貼齊道路側邊（並排） | 關閉後不吸到「緊貼在旁、不重疊」的平行位置 |
| 4 貼齊車道（重疊） | 關閉後不吸到「與既有道路重疊」的車道級位置 |
| 5 貼齊引導線 | 關閉後不會被任何引導線吸過去（線照樣可以顯示） |
| 6 貼齊鄰近路口 | 關閉後可在既有路口旁更近處接新節點 |
| 貼齊範圍 | 貼齊半徑與候選範圍倍率（0.25×–4×） |

| 引導線 | 作用 |
|---|---|
| 顯示引導線 | 總開關，只管畫不畫 |
| 角度導引線 + 角度格線 | ±45° / ±90° 那類斜線；格線可選遊戲預設 / 90° / 45° / 30° / 15° |
| 縱向導引線（原版） | 沿既有道路方向延伸的線 |
| 橫向導引線（新增） | 通過附近道路端點、與道路垂直的藍色虛線，讓幾條並排道路的端點切齊 |

取消某一種引導線 = 那種既不顯示也不吸附。「恢復遊戲預設」會關掉橫向導引線（那不是遊戲原本的功能）。

### 修改面板文字

面板文字一種語言一個檔：遊戲資料夾 `tf3snap\tf3snap_text_zh-TW.ini`、`_zh-CN.ini`、`_en.ini`、`_ja.ini`
（第一次啟動時自動產生）。一行一個 `代號 = 顯示文字`，只改等號右邊，用 UTF-8 存檔，**遊戲開著也會在一秒內更新**。
刪掉某一行就恢復該語言的內建文字；要換行寫 `\n`。面板寬度會跟著文字自動調整。

## 常見問題

| 問題 | 處理方法 |
|---|---|
| 遊戲裡看不到面板 | 要進入存檔（不是主選單）並打開道路或鐵路工具；面板最下面的勾選取消後會一直顯示。仍看不到就執行安裝檔選 **3** 產生報錯檔 |
| 面板顯示紅字「未掛鉤」 | 遊戲更新後對不上，外掛已自動停用貼齊功能，遊戲本身不受影響；請等新版 |
| 安裝時說遊戲還在執行 | 完全關閉遊戲；在工作管理員確認 `TransportFever3.exe` 已結束後再試 |
| 找不到遊戲資料夾 | 把含 `TransportFever3.exe` 的資料夾拖進安裝視窗，按 Enter |
| 防毒軟體警告 | 本工具會讓遊戲載入自己的 DLL 並修改記憶體中的遊戲函式，可能被誤判；原始碼都在這個倉庫，可自行編譯 |
| 遊戲崩潰（Internal error） | 先確認是 0.3.1 以上；執行安裝檔選 **3**，回報時附上 `報錯.log` |

回報問題請到 [Issues](https://github.com/Hsuamber/Transport-Fever-3-Better-Snap-Option-mod/issues)，附上版本、發生步驟和 `報錯.log`。
安裝失敗時會自動在安裝檔旁邊產生 `報錯.log`；檔案裡有電腦上的資料夾路徑（可能含使用者名稱），公開貼出前可以先看一下。

## 資料保存位置

以下路徑都相對於遊戲資料夾：

| 內容 | 路徑 |
|---|---|
| 外掛本體 | `vulkan-1.dll`、`tf3snap\tf3snap.dll` |
| 設定（各開關、面板位置、語言） | `tf3snap\tf3snap.ini` |
| 面板文字 | `tf3snap\tf3snap_text_<語言>.ini` |
| 紀錄 | `tf3snap\tf3snap.log`（這次）、`tf3snap\logs\`（之前 4 次） |
| 安裝紀錄 / 被替換檔案的備份 | `tf3snap\install.json` / `tf3snap\backup\` |
| 暫時停用開關 | `tf3snap\disabled`（空檔） |

## 運作方式

`vulkan-1.dll` 是一個轉接器：所有 Vulkan 呼叫都轉給系統原本的 `vulkan-1.dll`，只額外載入 `tf3snap\tf3snap.dll`。
外掛在記憶體裡掛上遊戲的游標貼齊函式，面板是獨立的置頂小視窗，和遊戲的繪圖無關。遊戲更新後若對不上，外掛不掛任何鉤子、
完全不碰遊戲，只在面板顯示紅字。

## 從原始碼編譯

一般玩家直接下載安裝包即可。需要 [llvm-mingw](https://github.com/mstorsjo/llvm-mingw/releases)、
[Dear ImGui](https://github.com/ocornut/imgui/releases) 1.92.9b、[MinHook](https://github.com/TsudaKageyu/minhook/releases) 1.3.4，
解壓到 `toolchain\` 後執行 `build.bat`，再用 `package.bat` 產生安裝包。詳細步驟與開發紀錄見 [STATUS.md](STATUS.md)。

## 免責聲明

本專案為非官方工具，與 Urban Games / Good Shepherd 無隸屬、合作或授權關係。這是讓遊戲載入自己的 DLL、在記憶體中修改遊戲函式的工具；
**請勿用於連線遊玩**，記憶體修改可能被視為作弊。使用可能帶來崩潰或存檔損壞等風險，不保證相容性與穩定性，請自負風險並記得備份存檔。

## 授權

本工具原始碼依 [MIT 授權](LICENSE) 釋出，Copyright (c) 2026 Hsuamber。
使用 [Dear ImGui](https://github.com/ocornut/imgui)（MIT）與 [MinHook](https://github.com/TsudaKageyu/minhook)（BSD 2-Clause），
授權原文附在安裝包的 `licenses\`。
