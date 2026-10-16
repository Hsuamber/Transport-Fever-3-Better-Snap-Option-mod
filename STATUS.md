# 更好的吸附貼齊 Better Snap Option（0.2.x：TF3 Snap Options，原生版 B）— 現況與接手說明

`README.md`（GitHub 首頁）只寫給玩家看的內容；開發者需要的放在本檔開頭的「開發者參考」，之後是逐版開發紀錄。
這個資料夾（`原始碼\`）就是 git 倉庫，`.gitignore` 排除工具鏈、build、本機設定。逆向筆記與分析工具（原本的
`<遊戲>\re\`）在專案的另一個資料夾 `..\分析工具\`，不在倉庫裡；下文提到的 `分析工具\`、`re/` 都指那裡。

方案 B：外掛（由 `vulkan-1.dll` 代理自動載入）用 MinHook 從特徵碼掛貼齊鉤子，面板是自己的
置頂 ImGui 視窗（和遊戲繪圖 API 無關），用 `g_gameUI` / ESC 暫停旗標自動隱藏。

## 開發者參考（2026-10-16 從 README 移過來）

### 資料夾結構

```
原始碼\                      ← 這個倉庫
├─ README.md                 GitHub 首頁（玩家向）
├─ LICENSE                   MIT 授權（package.bat 會一起打包）
├─ build.bat                 編譯 → build\vulkan-1.dll + build\tf3snap.dll
├─ deploy.bat                把 build\ 直接裝進遊戲（開發測試用）
├─ package.bat               把 build\ 打包成發布包 → ..\成品\
├─ src\plugin\               外掛本體 tf3snap.dll（C++：鉤子、面板、設定、文字、log）
├─ src\proxy\                vulkan-1.dll 代理（C：轉發 Vulkan、載入外掛）
├─ release\                  發布包裡的檔案：安裝-Install.bat、tools\install.ps1、四語 README.txt、licenses\
├─ tools\                    package.ps1、deploy.ps1、gen_sigs.py（特徵碼）、gen_proxy.py（代理匯出表）
├─ STATUS.md                 本檔
│
│  以下只在本機，不放進 git（見 .gitignore）：
├─ toolchain\                編譯工具：llvm-mingw、Dear ImGui、MinHook（約 720 MB）
├─ build\                    build.bat 的輸出與中間檔
└─ deploy.local.txt          deploy.bat 要裝進哪個遊戲資料夾
..\成品\                     package.bat 的輸出（發布到 GitHub Releases，不放進 git）
..\分析工具\                 逆向分析筆記與工具（作者本機，不在這個倉庫）
```

程式註解裡的 `re/NOTES.md`、`re/tools/...` 指 `..\分析工具\` 裡的檔案。發布包裡給玩家的說明是 `release\README.txt`
（四語、UTF-8 含 BOM + CRLF，記事本可直接開）；改了安裝步驟要同時改它和 README.md。

### 編譯與發布

`toolchain\` 要自己放三個資料夾（名稱一致，或用環境變數 `TC` / `IMGUI` / `MINHOOK` 指到別處）：
`llvm-mingw-20261006-ucrt-x86_64\`（[llvm-mingw](https://github.com/mstorsjo/llvm-mingw/releases)）、
`imgui-1.92.9b\`（[Dear ImGui](https://github.com/ocornut/imgui/releases)）、`minhook-1.3.4\`
（[MinHook](https://github.com/TsudaKageyu/minhook/releases)）。`tools\gen_sigs.py` 要 Python 3 + `pip install pefile capstone`。

1. **`build.bat`**：編譯兩個 DLL 到 `build\`。缺了哪個工具會直接說缺哪個檔；編譯訊息在 `build\build.log`。
   版本號在 `src\plugin\version.h`（`TF3SNAP_VERSION` 與 `TF3SNAP_VERSION_RC` 兩行一起改）。
2. **`deploy.bat`**：用正式的安裝程式把 `build\` 裝進遊戲。遊戲資料夾記在 `deploy.local.txt`（第一次會問，可拖入資料夾）。
   也可以 `deploy.bat -GameDir "D:\Games\Transport Fever 3"`、`deploy.bat -Action Report`（產生報錯檔）。
3. **`package.bat`**：產生 `..\成品\Better-Snap-Option-v<版本>\` 與同名 `.zip`（安裝 .bat、`tools\install.ps1`、兩個 DLL、
   四語 `README.txt`、`licenses\`、`LICENSE`、`SHA256SUMS.txt`）。`build\` 的 DLL 版本或名稱和 `version.h` 不同時會拒絕打包
   （`-Force` 才會照打，僅供測試）。旁邊沒有 `成品` 資料夾時改放這裡的 `dist\`。
4. 到 GitHub 建 Release（tag `v<版本>`），把 zip 傳上去。README 的下載連結指向 `releases/latest`，不用跟著改。

三支 .bat 失敗時都會停在畫面上顯示原因，並回傳非 0 的結束碼。編碼規則：`.ps1` 與 `release\README.txt` 是 UTF-8 含 BOM、
.bat 內容只用 ASCII、C++ / Python / Markdown 是 UTF-8 無 BOM；`.gitattributes` 讓 git 原樣保存每個檔案。

跨遊戲版本：位址用特徵碼掃描（`src\plugin\sigs.inc`，由 `tools\gen_sigs.py "<TransportFever3.exe>"` 重新產生）。
找不到符合的特徵碼時外掛不掛任何鉤子、完全不碰遊戲，面板會顯示紅字提示。

### 安裝程式（`release\tools\install.ps1`）

- 遊戲資料夾自動找：安裝包上層（解壓在遊戲資料夾裡時）、Steam 註冊表 + `libraryfolders.vdf` 裡的每個遊戲庫；
  找不到就請使用者把資料夾拖進視窗。選錯選項或資料夾會重問（3 次）。
- 檢查：真的有 `TransportFever3.exe`、遊戲沒在執行、兩個 DLL 都是 x64 而且確認是本外掛的（看 DLL 版本資訊的
  ProductName：`Better Snap Option`，或舊名 `TF3 Snap Options`；0.2.1 以前看只有它們才有的字串）。
- 遊戲資料夾已有別人的 `vulkan-1.dll`：先說明是什麼檔、問要不要繼續，備份到 `tf3snap\backup\vulkan-1.<時間>.dll`
  （驗證雜湊），記進 `tf3snap\install.json`；移除時自動還原。
- 先把新檔複製成 `*.tf3new` 並驗證 SHA256，再一起換上；中途失敗就把換過的還原。
- 更新不碰 `tf3snap.ini`、`tf3snap_text_*.ini`、log。移除時問要不要連設定一起刪（預設保留）。
- 介面語言：已安裝過就跟面板設定（`lang=`），否則跟 Windows 顯示語言。
- 報錯檔：任何失敗都會自動在 .bat 旁邊寫 `報錯.log`（寫不了就寫到 `%TEMP%`），選 3 也會產生。內容：安裝過程、
  錯誤與堆疊、系統 / PowerShell 版本、兩邊 DLL 的版本與雜湊、遊戲 exe 雜湊、遊戲資料夾其他 DLL、`tf3snap.ini` /
  `proxy_error.txt` / 最近 5 次遊玩的 log、遊戲自己的 `crash_dump`、Windows 14 天內這個遊戲的崩潰事件。
- 參數（不問問題）：`-Action Install|Uninstall|Report -GameDir <資料夾> -SourceRoot <來源> -Yes -RemoveData`。
  帶引號的路徑結尾不要加 `\`（`"D:\Games\TF3\"` 的 `\"` 會被當成跳脫的引號）。

### 紀錄（log）與面板文字檔

- `tf3snap\tf3snap.log`：這一次遊玩，第一行 `=== Better Snap Option session <日期時間> ===`（0.2.2 是 `TF3 Snap Options`）。
- `tf3snap\logs\tf3snap_<開始時間>.log`：之前的遊玩，只留最新 4 個（加上目前 = 最近 5 次）；新舊兩種檔頭都認得。
- `tf3snap\proxy_error.txt`：只有 `vulkan-1.dll` 載入不了 `tf3snap.dll` 時才會出現。
- 0.3.0 起，`tf3snap_text_*.ini` 裡還是舊版內建標題的行（與舊範本檔頭）在第一次啟動時自動換成新名字，改過的行不動。

## 已完成並通過語法檢查（`clang++ -fsyntax-only`）

| 檔案 | 內容 |
|---|---|
| `src/proxy/` | 代理 `vulkan-1.dll`（265 匯出轉發系統版；只在 TransportFever3.exe 載入 `tf3snap\tf3snap.dll`；放 `tf3snap\disabled` 空檔停用）。**此部分已實際編譯成功過。** |
| `src/plugin/sigs.inc` | 18 個目標函式特徵碼（`tools/gen_sigs.py` 產生，皆驗證在 .text 唯一） |
| `src/plugin/sigscan.cpp` | 特徵碼掃描 + 解析；找不到就不掛，不碰遊戲 |
| `src/plugin/gamestate.cpp` | 讀 `g_gameUI`(0x403c008) 與 ESC 旗標(+0xb90)，VirtualQuery 保護 |
| `src/plugin/hooks.cpp` | 貼齊 6 項 + 範圍、引導線（顯示/貼齊分流、角度格線、橫向導引線）的 MinHook 鉤子，逐行對照已實測的 Frida 版；ABI 參數型別按反編譯簽章對好 |
| `src/plugin/overlay.cpp` | 自有 D3D11 + ImGui 置頂視窗、自動隱藏、繁中字型 |
| `src/plugin/config.cpp` | `tf3snap.ini` 讀寫（含 0.1 版鍵名轉換、面板位置） |
| `src/plugin/text.cpp` | 面板文字，四種語言各一檔 `tf3snap_text_<代號>.ini`（缺檔寫範本、存檔即重載） |
| `src/plugin/log.cpp` | `tf3snap.log`（目前這次）+ `logs\tf3snap_<開始時間>.log`（之前 4 次） |
| `src/plugin/main.cpp` | DllMain → 初始化執行緒 → 掛鉤 + 開面板 |
| `build.bat` | 三步編譯（proxy / minhook / plugin），引用同一層 `toolchain\` 下的 llvm-mingw、ImGui 1.92.9b、MinHook 1.3.4（0.2.x 時在 `re/toolchain`） |

## 尚未驗證（我無法自己編譯 — 那步被安全防護擋下，由你按 build.bat）

- 整個外掛的 plugin 原始碼都過了 `-fsyntax-only`，但**連結階段（ImGui/MinHook 一起編譯連結、D3D11 連結）尚未驗證**，
  第一次 `build.bat` 可能還有連結錯或執行期小問題要修。
- 掛鉤邏輯本身邏輯等同已實測的 `tools/snaptool.py`，但「原生 MinHook 版在遊戲裡實跑」沒測過。

## v0.2.0（2026-10-15）：已從 Frida 版移植，語法檢查通過，尚未編譯

使用者在 Frida 版實玩確認後要求移植，並要求**面板文字放外部檔案**以便自己修改：

- 面板改成「貼齊 / 引導線」兩塊（同 Frida 版）；舊的「貼齊 90 度角」併進角度格線 90°；0.1 版設定檔自動轉換
  （`guides`→`snapGuides`、`angle90=1`→`grid=90`）。
- 3/4 拆分（並排 / 車道）、顯示與貼齊分開（`GetSnapTangents` 依返回位址分流）、橫向導引線（自由點 + 並排偏移點）。
- 新增 `text.cpp`：`tf3snap\tf3snap_text.ini`，缺檔時寫出範本，每秒檢查修改時間、存檔即更新；ImGui 以 `文字##代號`
  當 id，改字不影響狀態。Win32 視窗跟著 ImGui 自動尺寸縮放；標題列可拖曳，位置存 `panelX/panelY`。
- 特徵碼 11 → 18 個（新增 7 個為「可選」：找不到只會停用橫向導引線，面板顯示橙字，其餘照常）；
  `BaseEdge` 的 `type_info` 執行期用 RTTI 名稱 `.?AUBaseEdge@component@ecs@@` 在 `.data` 找（不寫死資料位址）；
  兩個呼叫點返回位址（圓錐 `+0x221`、畫導引線 `+0x3b2`）會驗證那裡真的是 `call` 到目標函式。
- **修正 0.1 版 bug**：`FindSnapPointBaseEdge` 的 detour 把參數位置算差一格（把 node2seg 當成游標、reference 當成起點），
  選項 1、2 在原生版會讀錯資料；現在與 Frida 版一致（cursor = 第 10 個、start = 第 13 個參數）。
- 橫向導引線加了防護：讀 node2seg / 元件前先 `Readable()`；元件索引不合理時本次遊戲停用該功能並寫 log。
- 尚未驗證：連結、實際在遊戲中跑（同 0.1 版，等使用者按 `build.bat`）。
- `build.bat` 修正（使用者第一次實跑時卡在第 2 步）：`if "%MINHOOK%"==""set` 少一個空白 → cmd 根本沒設 MinHook 路徑
  （已用小 bat 實測確認）；第 3 步的 `"%OUT%\mh\*.o"` 改成明確列出物件檔；換成 CRLF；編譯器輸出寫進
  `build\build.log`（失敗時印出來、結尾 `pause`），點到視窗進入「選取」模式也不會卡住編譯器。
- **第一次完整編譯成功（2026-10-15 10:15，使用者執行）**：零錯誤零警告。檢查產物：`vulkan-1.dll` 265 個匯出、只依賴
  KERNEL32/USER32；`tf3snap.dll` 卻依賴 `libc++.dll`、`libunwind.dll`（llvm-mingw 預設動態連結 C++ 執行庫，遊戲資料夾沒有
  → 會靜靜載入失敗）→ 第 3 步加 `-static`，另加 `-Wl,--exclude-all-symbols`（原本把 ImGui 等 1413 個符號全匯出）。
  需要重新編譯一次再檢查匯入表。
- **第一次遊戲內實跑（10:21）**：log 全綠（18 個特徵碼、`all hooks installed (cross guide on)`、文字範本寫出、`overlay: up`），
  拖曳標題列有效（存了 panelX/Y）；但面板縮成一條黑色豎條。原因：用了 `AlwaysAutoResize`，ImGui 會把自動尺寸限制在
  「顯示區」以內，而顯示區就是面板自己的 Win32 視窗 → 第一幀量到很小就縮下去、之後永遠長不回來。改成 ImGui 視窗填滿
  Win32 視窗、Win32 視窗依 `ContentSizeIdeal`（不受視窗大小限制的內容尺寸）調整。待重新編譯驗證。
- **第二次實跑（10:29–12:41）**：面板尺寸正常、可拖曳、改文字檔即時重載（25→23 行）、跑 2 小時穩定；橫向導引線有在跑
  （`BaseEdge component index 27`）。**12:41 用道路工具點擊時崩潰**：0xC0000005 在 `580f95`。原因：`CalcCursorPoint`
  以隱藏指標回傳結構，並把該指標放在 RAX 回傳；點擊路徑 `580900` 在呼叫後立刻讀 `[rax]`，而我的 detour 宣告成 `void`
  → RAX 是垃圾。（Frida 版會自動保留回傳值，所以沒事；0.1 版原生也有這個 bug，只是沒跑過。）修正：所有以隱藏指標
  回傳結構的 detour（CalcCursorPoint、MyFindSnapPointNew、CollectNodesAndEdges、GetSnapTangents、UpdateRenderer）
  都改成原樣傳回原函式的回傳值。順帶：toolkit 的 engine 指標每幀在兩個值間切換 → 元件索引只在值改變時記 log、
  候選道路快取只看時間；面板拿掉「取消某一種…」與「文字可在…修改」兩行提示（使用者要求，文字檔也不再有這兩個代號）。待重新編譯驗證。

## v0.2.1（2026-10-15 下午）：面板只在道路 / 鐵路工具開著時出現 — 語法檢查通過，尚未編譯

使用者要求「貼齊工具只應該在我點擊道路工具或是鐵路工具時才會出現」。

- 判斷方式：掛 `UI::StreetBuilder::Step`（585e50，特徵碼本來就在 `sigs.inc` 但沒用）只記時間戳。道路和軌道是同一個
  工具類別、工具開著時每幀呼叫一次（NOTES 7、10），500 ms 內有呼叫 = 工具開著。detour 型別 `(this, renderer, float dt)`
  （反組譯確認 `vmovaps xmm12, xmm2`、第一條指令剛好 5 bytes），一樣原樣傳回 RAX。
- 面板正在被使用時不收：已顯示 + （面板是前景視窗 或 滑鼠在面板上）→ 保留。避免遊戲失焦時降幀造成點面板時面板消失。
- 新勾選「只在道路 / 鐵路工具開啟時顯示」（`toolOnly`，預設開，存 `tf3snap.ini`）。
- **拿掉「只在遊戲中顯示」勾選**（使用者：開得了道路工具一定已經在遊戲中）。主選單那部分確實被工具偵測涵蓋；
  但 ESC 選單、切到別的程式時工具可能還開著，所以這兩項改成**一律生效、不再是選項**（`ShouldShow` 開頭）。
  `autohide` 從 Options / ini / 文字檔移除，舊 ini 的 `autohide=` 讀檔時忽略、下次存檔消失。
  特徵碼找不到時此勾選變灰、橙字提示、面板照舊一直顯示（不會永遠藏起來）。
- log 會記 `road / track tool opened / closed`，實跑一次就能確認偵測正確；`all hooks installed (... tool detection on)`。
- 文字新增 `toolOnly`、`status.notool`、移除 `autohide`（`text.cpp` 預設 + 使用者的 `tf3snap\tf3snap_text.ini` 已同步）。
- 版本號改 0.2.1，方便從面板標題看出跑的是不是新版。
- **未驗證（只有靜態推論）**：「Step 只在道路/軌道工具開著時被呼叫」——沒有在推土機、地形、車站等其他工具下實測。
  道路工具裡的「升級」分頁若是另一個工具類別，切過去面板也會收起（應該可以接受）。
- **注意**：14:00 遊戲載入的仍是 10:28 的舊 `tf3snap.dll`（12:45 的「道路工具點擊崩潰」修正還沒編進去）。

### 同版其他優化（使用者：「看看還有什麼能優化的處理一下」）

- **點面板不再搶走遊戲鍵盤**：視窗加 `WS_EX_NOACTIVATE`、`WM_MOUSEACTIVATE` 回 `MA_NOACTIVATE`。以前點一下勾選，
  Shift（精確模式）、ESC、C/T/G 等就送到面板，要再點回遊戲；道路工具開著時那一下還可能放下一個路點。
  只有「長度」輸入框在編輯時用 `SetForegroundWindow` 暫時拿鍵盤，Enter / 點別處結束後交回遊戲視窗
  （迴圈裡記住本行程最後一個非面板的前景視窗）。**未實測**：遊戲用 SDL2（只有一般 WM_ 滑鼠訊息、無 raw input
  匯入），推論點面板不會讓遊戲收到點擊，但要實玩確認。
- **特徵碼掃描快約 10 倍**：每個特徵碼用「最稀有的固定位元組」當錨點（取樣直方圖），SSE2 一次比 16 bytes。
  離線測試（`LoadLibraryEx(AS_IMAGE_RESOURCE)` 映射 exe，呼叫原 `ResolveTargets()`）：21 個位址全部與參考版相同、
  非唯一特徵碼仍拒絕；單一特徵碼 365–406 ms → 22–32 ms，全部 0.35–0.39 s（記憶體已熱）。測試程式在 scratchpad，
  不屬於外掛。`FindTypeInfo` 也改用 `memchr`。
- **設定檔延遲寫入**：以前拖滑桿每幀寫一次 `tf3snap.ini`（遊戲資料夾在 USB 碟）；現在停手 0.8 s、面板收起、
  或結束時才寫。
- **閒置時少畫**：滑鼠不在面板上、沒有按著元件、沒在打字時，改成有訊息或每 100 ms 才重畫（以前一直 60 fps）。
- `Logf` 加 printf 格式檢查屬性；全部呼叫都通過，沒有格式錯誤。

### 編譯檢查（14:35）

- 使用者 14:24:57–14:26:11 編譯成功（`build.log` 結尾 `== OK`），遊戲資料夾的兩個 DLL 與 `build\` 逐位元組相同；
  所有原始碼最後修改 14:21:32，早於編譯 → 全部改動都在。`tf3snap.dll` 內有 `0.2.1`、`StreetBuilderStep`、`toolOnly`，
  沒有 `autohide`；匯入表沒有 libc++ / libunwind（`D3DCOMPILER_47.dll` 是系統內建）；匯出 0 個。
- **新版還沒在遊戲裡跑過**：最新的 `tf3snap.log` 是 14:18 的 v0.2.0。
- 14:23 崩潰 = 同一個 `580f95` 舊 bug（14:18 那次用的是 v0.2.0）。14:15 崩潰 = **E: USB 碟斷線又接回**
  （14:15:28 Ntfs 98 重新掛載），不是外掛造成。`crashlog.py` 已會自動判讀這兩種情況。

## v0.2.2（2026-10-15 晚上）：面板語言選單 — 語法檢查 + 離線渲染測試通過，尚未編譯 DLL

使用者要求「在當前工具增加一個 list box 可以選擇要什麼語言，並做文字翻譯：簡中、英文、日文」。

- 面板最下面加「語言」下拉選單（繁體中文 / 简体中文 / English / 日本語，各以自己的語言顯示）。選單在視窗最底，
  彈出清單會自動往上開（ImGui 的顯示區就是面板視窗）。選完下一幀換字型與文字，`lang=` 存 `tf3snap.ini`。
- `text.cpp`：四份內建文字表（`kZhTW/kZhCN/kEn/kJa`，各 25 個代號，新增 `lang`）；一種語言一個檔
  `tf3snap_text_<代號>.ini`，缺的範本啟動時一次寫齊；舊的 `tf3snap_text.ini` 自動改名為 `_zh-TW.ini`（保留使用者改過的字）。
  繁中內建文字改成使用者在 `tf3snap_text.ini` 裡改過的較短說法，其他語言依此翻譯。
  簡中用大陸用語（吸附、辅助线、米、默认），不是單純繁簡轉換。
- 預設語言：沒有設定檔 → 跟 Windows 顯示語言（zh-CN/SG→簡中、其他中文→繁中、日文→日文、其他→英文）；
  有設定檔但沒有 `lang=`（0.2.1 以前）→ 繁中，不會突然變語言。
- 字型（`overlay.cpp` `kFaces`）：繁中 msjh[0]、簡中 msyh[0]、日文 Meiryo UI（meiryo.ttc[2]，行高比例 1.27 與正黑體相同，
  面板大小不變；退而求其次 Yu Gothic UI、MS UI Gothic）、英文 msjh[0]（拉丁字母即 Segoe UI 設計，語言選單裡的中日文名稱不用另外補字）。
  主字型缺語言選單需要的字（日文字型沒有「简」）時，自動合併 msyh 補字。用 fontTools 檢查過所有面板文字在選定字型都有字形。
  切語言時在兩幀之間 `ClearFonts()` 再載入（ImGui 1.92 支援）；字型路徑改用 `GetWindowsDirectoryW`；
  `ImFontFlags_NoLoadError`（檔案讀不了就換下一個，不會 assert）；不再傳 glyph ranges（1.92 動態字型集按需點陣化）。
- 角度格線下拉選單寬度改成依最長選項計算（英文「Game default」原本被截斷）。
- **離線測試**（scratchpad，不屬於外掛）：測試程式直接 `#include` 真正的 `overlay.cpp` + `text.cpp`，用真的 ImGui DX11 後端
  畫到離屏貼圖。結果：舊檔改名、三份範本寫出、四種語言來回切換 9 次無錯誤；模擬滑鼠點開語言選單（往上展開）、點「日本語」
  → `lang=ja`、設定標記為待存、字型換成 Meiryo UI + 雅黑。四語截圖 + 選單展開圖在 `preview_languages.png`。
- 未驗證：DLL 編譯（照舊由使用者按 `build.bat`）與遊戲內實跑。

## 發布用安裝程式（2026-10-16）— 腳本實測通過；DLL 版本資源只驗證到編譯成 .o

使用者看了 Endfield Poser 的 `安全安装.bat` + `deploy.ps1`（357 行，同樣是 vulkan-1.dll 代理架構）後要求做一個，
並「額外增加一個會自動產生報錯.log」。做的是精簡版（細節見 README「安裝」「發布」）：

- `release/安裝-Install.bat`（純 ASCII 內容）→ `release/tools/install.ps1`（UTF-8 BOM）：安裝 / 更新 / 移除 / 產生報錯檔，
  四語訊息，可互動也可帶參數。別人的 `vulkan-1.dll` 備份 + 移除時還原；先 stage 再換、失敗回滾；保留使用者檔案。
- 失敗自動寫 `報錯.log`（選單 3 也可手動產生），內容見 README。互動模式寫完會用檔案總管選取它。
- DLL 版本資源：`src/plugin/version.h`（`TF3SNAP_VERSION` 與 `TF3SNAP_VERSION_RC` 兩行一起改）、`src/plugin/tf3snap.rc`、
  `src/proxy/proxy.rc`；`build.bat` 用 `x86_64-w64-mingw32-windres` 編成 `build\*_res.o` 連進 DLL。
  兩個 .rc 已實際編譯成 COFF .o（有 `.rsrc`），**完整 DLL 連結還沒驗證**（等使用者跑 build.bat）。
- `deploy.bat`（開發用：把 `build\` 裝進遊戲）、`package.bat` → `tools/package.ps1`（`dist\` 資料夾 + zip，
  zip 內路徑用 `/`、中文檔名帶 UTF-8 旗標；版本不符拒絕打包）、`release/README.txt`（四語）、`release/licenses/THIRD-PARTY.txt`。
- **測試**（scratchpad 假遊戲資料夾，用 System32 的 vulkan-1.dll 當「別的模組」、現有 0.2.1 DLL 當安裝包）：
  有別人的 loader 時安裝 → 備份 + 紀錄；重裝 → 全部略過；更新 → 只換變了的檔、使用者 ini / 文字檔不動、訊息跟 `lang=ja` 變日文；
  移除 → 還原別人的 loader、設定保留；`-RemoveData` → 資料夾刪除；選錯資料夾 / 遊戲執行中（用改名的 PING.EXE 模擬）
  → 拒絕 + 自動 `報錯.log`（約 170 KB，各段都有內容）；互動模式用管線輸入測過選單、拖入的帶引號路徑、打錯重問；
  zip 解壓後直接跑裡面的 .bat 安裝 / 移除成功。互動模式的「報錯後開檔案總管」沒有測（會在使用者桌面跳視窗）。
- 專案還沒有 `LICENSE` 檔（README 寫 MIT）：`package.ps1` 會警告，版權人要使用者決定。

### log 改成保留最近 5 次（2026-10-16）

- 原因：只留 `tf3snap.log` + `.prev.log` 兩次時，崩潰後連開兩次遊戲那次的紀錄就沒了（`crashlog.py` 判讀 10/15 14:23
  那次時就只能說「外掛狀態不明」）。
- `log.cpp`：`tf3snap.log` 仍是目前這次（路徑不變），第一行寫入含日期的檔頭；啟動時把上一次移到
  `logs\tf3snap_<該次開始時間>.log`（舊格式 / `tf3snap.prev.log` 用最後寫入時間），只留最新 4 個。
  `MoveFile` 之後立刻建新檔會觸發 NTFS 檔名 tunneling（新檔沿用舊檔的建立時間），所以不用建立時間判斷。
- 離線測試（scratchpad 小程式直接 `#include` log.cpp）：升級情境兩個舊 log 正確歸檔；連跑 6 次後 `logs\` 剩 4 個 + 目前 1 個。
- 安裝程式報錯檔：收 `tf3snap.log`、`logs\` 最新 4 個、舊的 `tf3snap.prev.log`（測試確認最舊的第 5 個不收）。
- `re/tools/crashlog.py` 的 `native_logs()` 讀全部三種位置，有檔頭時用檔頭日期當開始時間；新舊格式都測過。

## v0.3.0（2026-10-16）：改名 Better Snap Option + 專案搬到桌面 — 語法檢查與離線測試通過，尚未編譯 DLL

使用者要求：工具改名「更好的吸附貼齊 / Better snap option」；專案資料夾分成編譯前（`原始碼\`）與編譯後（`成品\`）；
遊戲資料夾裡編譯前的內容搬進 `原始碼\`；根目錄放 README.md。

- **搬移**：`<遊戲>\re\tf3-snap` 複製到 `原始碼\`（不含 `build\`、`dist\`，30 檔雜湊比對一致），原本那份移到
  `<遊戲>\re\backup\tf3-snap-before-move-20261016\`（含當時的 build\ / dist\）。`re\toolchain` 的 llvm-mingw、ImGui、
  MinHook 複製到 `原始碼\toolchain\`（9391 檔、位元組數一致；`re\toolchain` 原地保留，`dl\` 原始壓縮檔沒帶）。
  `re\NOTES.md`、`re\tools\`（含 Frida 版）留在遊戲資料夾，是逆向工作區不是本工具原始碼。
  0.2.2 發布包移到 `成品\TF3-Snap-Options-v0.2.2\`（+ zip，SHA256SUMS 驗證一致）。原 `README.md` 併入根目錄 README。
- **改名範圍**：`version.h` 的 `TF3SNAP_PRODUCT`（→ 兩個 .rc 的 ProductName、main.cpp 啟動行、config.cpp 設定檔頭、
  log.cpp 檔頭、text.cpp 範本檔頭都改用這個巨集）、兩個 .rc 的 FileDescription、overlay.cpp 視窗標題、text.cpp 四語 `title`
  （繁「更好的吸附貼齊」、簡「更好的吸附贴齐」、英日「Better Snap Option」）、install.ps1 四語標題與報錯檔標題、
  package.ps1 產品名與包名 `Better-Snap-Option-v<版本>`、release README.txt / THIRD-PARTY.txt / 安裝 .bat 註解、build.bat。
  版本 0.2.2 → **0.3.0**。**檔名不改**（`tf3snap.dll`、`tf3snap\`、ini、log），升級路徑因此不變。
- **相容舊名**：install.ps1 `$LegacyProducts = @('TF3 Snap Options')`，`Test-Ours` 與 `Read-Record` 都接受；
  log.cpp `kOldHeader` 讓 0.2.2 的 log 歸檔時仍用檔頭日期命名；`re/tools/crashlog.py` 的 `PLUGIN_NAMES` 兩種名字都認；
  text.cpp `kRetired` + `UpgradeFile()`：文字檔裡還是舊內建標題的行、以及舊範本檔頭，在 `TextInit` 時換成新的
  （先寫 `.tmp` 再 `MoveFileExW` 取代；保留 BOM 與換行；使用者改過的行不動）。
- **路徑**：build.bat 預設 `%HERE%toolchain\...`，開頭先檢查 clang++.exe / imgui.cpp / MinHook.h，缺了寫進 log 並以 2 結束
  （整支照舊回 1）；package.ps1 授權檔改從 `原始碼\toolchain` 拿，輸出到 `..\成品`（沒有就 `dist\`）；deploy.bat 改呼叫新的
  `tools\deploy.ps1`：遊戲資料夾記在 `deploy.local.txt`（UTF-8，第一次問、可拖入、驗證有 exe），再帶 `-GameDir` 叫
  install.ps1。package.bat / deploy.bat 改成回傳 PowerShell 的結束碼（原本被 `pause` 蓋成 0）。
- **驗證**（全部從含中文與空白的路徑跑）：8 個 plugin .cpp + proxy .c `-fsyntax-only` 通過；兩個 .rc 用 windres 編成 .o，
  字串只有新名字與 0.3.0。離線小程式 `#include` 真的 text.cpp + log.cpp，對使用者遊戲資料夾裡 0.2.2 產生的文字檔與 log
  的複本跑：en / zh-TW 只改檔頭與 title 兩行、改過 title 且帶 BOM 的 zh-CN 只改檔頭且 BOM 保留、缺的 ja 寫新範本、
  0.2.2 log 歸檔成 `tf3snap_2026-10-16_164426.log`；第二次跑文字檔完全不變。crashlog.py `native_logs()` 新舊格式各讀到一筆。
  安裝程式（scratchpad 假遊戲資料夾 = 使用者目前 0.2.2 安裝的複本；當時使用者在玩遊戲，測試複本把「遊戲執行中」檢查拿掉）：
  更新 → 認得舊名 DLL 與舊 `install.json`、略過相同檔、紀錄改寫成新名；報錯檔標題新名、兩個 DLL「ours」；移除 → 刪 DLL 留設定。
  實際入口（scratchpad 複本）：build.bat 缺工具鏈 → 顯示缺的檔、結束碼 1；package.bat 沒 build → 1；`-Force` 假 build → 
  輸出到旁邊的 `成品\Better-Snap-Option-v0.3.0\`；deploy.bat 第一次問路徑並記住、第二次直接用、錯路徑 → 1。
- **未驗證**：DLL 編譯與連結（照舊由使用者按 `build.bat`）、遊戲內實跑（面板標題、文字檔自動更新）、
  `preview_languages.png` 還是 0.2.2 舊標題的截圖。

### 逆向工作區搬進 `分析工具\`（2026-10-16 晚上）

- 使用者要求把遊戲資料夾 `re\` 裡的工具、筆記等搬到 `原始碼與分析工具\`，然後刪掉 `re\`。搬進 `分析工具\`（不和外掛自己的
  `tools\` 混在一起）：NOTES.md、tools\、cache\、decomp\、logs\、ghidra\、三支 .bat、崩潰報告.txt（4785 檔，大小全對、
  Ghidra 以外 138 檔雜湊一致）。沒搬：`toolchain\`（同一份已在 `原始碼與分析工具\toolchain`，只少了 `dl\` 原始壓縮檔）、
  `backup\`（三份今天的備份）、`__pycache__`。`re\` 整個送進資源回收筒（沒有永久刪除）。
- 路徑：`toollog.find_game_dir()`（`TF3_GAME_DIR` → `分析工具\game-dir.local.txt` → 舊的「上兩層」），`tf3.py`、`crashlog.py`
  改用它；`用工具啟動遊戲.bat` 改叫新的 `tools\start_game.py`（找不到遊戲時跳訊息框）。
- **Ghidra 11.1.2 的 Jython 不能處理中文路徑**（`Cannot create PyString with non-byte value`；log4j 設定也讀不到，
  於是腳本的 println 全部消失）。`gplan.py` 改成把 DecompPlan.py、plan.json、輸出放在 `%TEMP%\gplan_*`（ASCII），
  跑完從那裡收 .c 檔再刪掉暫存。驗證：從新位置反編譯 60c360，結果和舊 `decomp\` 那份只差標頭多一個空行。
- 也從新位置驗證過：19 支 .py 編譯通過、`toollog.GAME_DIR` = 遊戲資料夾、`strs.py` 讀 cache、`crashlog.py` 產生報告。
  沒驗證：`snaptool.py` / `hook.py` / `live.py`（要掛到執行中的遊戲）與 `start_game.py` 真的開遊戲。

## v0.3.1（2026-10-16 晚上）：修正密集地區開道路工具就崩潰（heap corruption）— 離線測試通過，尚未編譯

- **現象**：使用者自己編好 0.3.0、裝好，讀進一個高速公路密集的存檔（first1，17 個模組），19:54:27 打開道路工具，
  1.4 秒後「Internal error」。崩潰 0xC0000409；快照例外 0xC0000374（heap corruption），主執行緒
  `UI::StreetBuilder::Step` → `tf3snap.dll+0x4be2`（呼叫遊戲的 operator delete）→ `RtlFreeHeap` 偵測到。
- **原因**：橫向導引線的 `CollectLines` 用 `oOpDel(vec.b, cap - b)` 釋放 CollectNodesAndEdges 填好的 `vector<int32>`。
  但 MSVC 的 std::allocator 對 **≥ 4096 bytes** 的區塊會多配 0x27、使用者指標對齊 32、把真正的區塊起點存在 `ptr[-1]`；
  釋放時要先還原（反編譯的 FindSnapPointBaseEdge 就是 `if (0xfff < size) { size += 0x27; real = *(ptr - 8); ... }`）。
  我們直接 delete 使用者指標 = 把區塊中間的位址交給 heap。cone 查詢超過 1024 個實體才會發生：minidump 主執行緒堆疊上
  就有一個 1536 個 int、6144 bytes、起點 32 對齊的 vector。0.2.0 起就有這個 bug，0.2.2 玩 2 小時沒事是因為那張地圖沒那麼密。
  Frida 版 `snaptool.py` 也有同樣的寫法（cone 查詢、角度格線、自我測試三處）。
- **修正**：hooks.cpp 新增 `GameVecAlloc` / `GameVecFree`（照 MSVC 的規則；檔頭不對就寧可漏掉不釋放並記一次 log），
  `CollectLines` 與 `filterCandidates`（角度格線換新 buffer、釋放舊的）全部改用；snaptool.py 加同樣的 `gameVecAlloc` /
  `gameVecFree`。版本 0.3.0 → 0.3.1。
- **驗證**：hooks.cpp `-fsyntax-only` 通過；snaptool.py 的 Frida 腳本 `node --check` 通過。離線測試從 hooks.cpp 原文切出
  兩個函式、模擬 MSVC allocator：4 B 到 1.7 MB 共 11 種大小 ×（遊戲配我們放 / 我們配遊戲放 / 我們配我們放）全部正確、
  沒有殘留區塊；檔頭不對的區塊有漏掉並記 log；舊寫法在 6144 bytes 上重現「delete 不是區塊起點」。
- **使用者實測（20:04 起）**：自己編譯、打包、安裝 0.3.1，同一個存檔開道路 / 鐵路工具 27 次、橫向導引線開著，沒有再崩潰。

### 準備上 GitHub（2026-10-16 晚上）

- README.md 從專案根目錄搬進這裡、改寫成 GitHub 首頁（玩家：下載 / 安裝 / 功能；開發：結構、準備工具鏈、編譯發布、版本紀錄）。
  發布包仍附 `release\README.txt`（四語、記事本可直接開；README.md 是開發者向、只有中文）。
- `.gitignore`：toolchain、build、dist、`*.local.txt`、`分析工具\` 的 ghidra（內含遊戲 exe）/ cache / decomp（由遊戲產生）/
  logs / 崩潰報告.txt、snaptool 的設定與除錯檔。以暫存 git 目錄檢查：會進 git 的是 56 檔、731 KB，裡面沒有使用者名稱或電子郵件。
- `toollog.py` 原本寫死本機的遊戲使用者資料夾；改成 `find_local_dir()`，和安裝程式報錯檔一樣搜尋
  `%PUBLIC%\Documents\Steam\*\3493540\local` 與 `Steam\userdata\*\3493540\local`（`TF3_LOCAL_DIR` 可指定），NOTES.md 同步改寫。
  驗證：本機找到的是同一個資料夾，crashlog 照常列出崩潰。
- 使用者決定：`LICENSE` = MIT，版權人 Hsuamber；`原始碼與分析工具\` 拆成專案底下的 `原始碼\`（git 倉庫）與
  `分析工具\`（不公開），`.gitignore` 拿掉分析工具那幾行。`preview_languages.png`（0.2.2 舊標題截圖）由使用者刪除，
  README 不再引用。倉庫：https://github.com/Hsuamber/Transport-Fever-3-Better-Snap-Option-mod （Private）。

## 建議驗證順序

1. `toolchain\`（llvm-mingw、imgui-1.92.9b、minhook-1.3.4）已放在同一層，執行 `build.bat`，清掉連結錯。
2. 把 `build/vulkan-1.dll` 複製到遊戲資料夾、`build/tf3snap.dll` 放進遊戲資料夾的 `tf3snap\`。
3. 開遊戲，看 `tf3snap\tf3snap.log`：應有 `all hooks installed`、`overlay: up`。
4. 進遊戲測各選項（對照 Frida 版行為應一致），特別是橫向導引線（log 應有 `cross guide targets: all found`、
   `BaseEdge component index N`）；測主選單 / ESC / 切出視窗時面板是否自動隱藏。
5. 改目前語言的 `tf3snap\tf3snap_text_<代號>.ini` 某一行存檔，面板應在一秒內換字、寬度跟著變；
   用最下面的「語言」選單切四種語言（log 會記 `font: ja -> meiryo.ttc[2] + msyh.ttc` 之類）。

參考位址與行為全在遊戲資料夾的 `re/NOTES.md`；Frida 參考實作在 `re/tools/snaptool.py`。
