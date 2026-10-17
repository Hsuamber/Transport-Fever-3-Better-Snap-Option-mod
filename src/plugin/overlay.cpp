// The panel: a small always-on-top window with its own D3D11 device + Dear ImGui.
// It does NOT touch the game's renderer (renderer-agnostic), shows only while the road / track tool is
// open and the game is in front (see HideReason).  Adapted from imgui's example_win32_directx11.
#include "tf3snap.h"

#include <d3d11.h>
#include <dxgi.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

#define IMGUI_DEFINE_MATH_OPERATORS   // required before imgui.h when imgui_internal.h is used
#include "imgui.h"
#include "imgui_internal.h"     // ContentSizeIdeal: the panel sizes its own Win32 window
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace tf3 {

void SaveConfig();

static ID3D11Device* g_dev = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;
static IDXGISwapChain* g_swap = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static HWND g_hwnd = nullptr;
static HWND g_gameHwnd = nullptr;   // last foreground window of the game (our process, not the panel)
static volatile bool g_stop = false;
static bool g_dirty = false;   // options changed -> save (debounced in OverlayRun)

static void CreateRTV() {
    ID3D11Texture2D* bb = nullptr;
    if (SUCCEEDED(g_swap->GetBuffer(0, IID_PPV_ARGS(&bb))) && bb) {
        g_dev->CreateRenderTargetView(bb, nullptr, &g_rtv);
        bb->Release();
    }
}

static bool CreateDevice(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    UINT flags = 0;
    D3D_FEATURE_LEVEL fl;
    const D3D_FEATURE_LEVEL want[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, want, 2,
                                             D3D11_SDK_VERSION, &sd, &g_swap, &g_dev, &fl, &g_ctx)))
        return false;
    CreateRTV();
    return true;
}

static void CleanupDevice() {
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
    if (g_swap) { g_swap->Release(); g_swap = nullptr; }
    if (g_ctx) { g_ctx->Release(); g_ctx = nullptr; }
    if (g_dev) { g_dev->Release(); g_dev = nullptr; }
}

static LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) return true;
    switch (msg) {
        case WM_SIZE:
            if (g_dev && wp != SIZE_MINIMIZED && g_swap) {
                if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
                g_swap->ResizeBuffers(0, (UINT)LOWORD(lp), (UINT)HIWORD(lp), DXGI_FORMAT_UNKNOWN, 0);
                CreateRTV();
            }
            return 0;
        case WM_MOUSEACTIVATE:
            // Clicking the panel must not take the keyboard from the game: Shift (precision), ESC, C/T/G ...
            // would stop working, and the click needed to get focus back could place a road point.
            return MA_NOACTIVATE;
        case WM_CLOSE:
            // hide instead of destroy: the game is still running; the panel is dismissed, not unloaded
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        case WM_DESTROY:
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// Fonts, per panel language: the first installed face of the list.  Each CJK language gets its own face so
// the shared Han characters take that language's shapes.  English uses JhengHei / YaHei, whose Latin letters
// are Segoe UI's, so the CJK names in the language list need nothing extra.  Meiryo UI has the same line
// height ratio as JhengHei (1.27), so the panel keeps its size; it lacks Simplified-only characters ("简" in
// the language list), which a merged fallback face supplies.
struct FontFace { const wchar_t* file; int index; };   // index: face inside a .ttc
static const FontFace kFaces[kLangCount][4] = {
    /* zh-TW */ {{L"msjh.ttc", 0}, {L"msyh.ttc", 0}, {L"simsun.ttc", 0}},
    /* zh-CN */ {{L"msyh.ttc", 0}, {L"msjh.ttc", 0}, {L"simsun.ttc", 0}},
    /* en    */ {{L"msjh.ttc", 0}, {L"msyh.ttc", 0}, {L"segoeui.ttf", 0}},
    /* ja    */ {{L"meiryo.ttc", 2}, {L"YuGothM.ttc", 1}, {L"msgothic.ttc", 1}, {L"msjh.ttc", 0}},
};
static const FontFace kFallbackFaces[] = {{L"msyh.ttc", 0}, {L"msjh.ttc", 0}, {L"simsun.ttc", 0}};
static constexpr float kFontPx = 18.0f;

static ImFont* AddFace(const FontFace& f, bool merge) {
    wchar_t dir[MAX_PATH];
    const UINT n = GetWindowsDirectoryW(dir, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return nullptr;
    const std::wstring w = std::wstring(dir) + L"\\Fonts\\" + f.file;
    if (GetFileAttributesW(w.c_str()) == INVALID_FILE_ATTRIBUTES) return nullptr;
    char path[MAX_PATH * 3];
    if (!WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, path, sizeof(path), nullptr, nullptr)) return nullptr;
    ImFontConfig cfg;
    cfg.FontNo = (ImU32)f.index;
    cfg.MergeMode = merge;
    cfg.Flags |= ImFontFlags_NoLoadError;   // unreadable file: return null instead of asserting
    return ImGui::GetIO().Fonts->AddFontFromFileTTF(path, kFontPx, &cfg);
}

// Every character of the language list, which shows all languages whatever the panel language is.
static bool CoversLanguageNames(ImFont* font) {
    for (int l = 0; l < kLangCount; ++l) {
        const char* s = LangName(l);
        const char* end = s + strlen(s);
        while (s < end) {
            unsigned int c = 0;
            s += ImTextCharFromUtf8(&c, s, end);
            if (c && !font->IsGlyphInFont((ImWchar)c)) return false;
        }
    }
    return true;
}

// Between frames only (before NewFrame).  Glyphs are rasterised on demand (ImGui 1.92 dynamic atlas), so
// no glyph ranges are needed; the files stay loaded until the next switch.
static void LoadFont(int lang) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->ClearFonts();
    ImFont* font = nullptr;
    const FontFace* used = nullptr;
    for (const FontFace& f : kFaces[lang]) {
        if (!f.file) break;
        if ((font = AddFace(f, false))) { used = &f; break; }
    }
    if (!font) {
        io.Fonts->AddFontDefault();   // last resort: Latin only (CJK shows as boxes)
        Logf("font: no CJK font found for %s, using ImGui's default", LangCode(lang));
        return;
    }
    const FontFace* merged = nullptr;
    if (!CoversLanguageNames(font))
        for (const FontFace& f : kFallbackFaces)
            if (_wcsicmp(f.file, used->file) != 0 && AddFace(f, true)) { merged = &f; break; }
    Logf("font: %s -> %ls[%d]%s%ls", LangCode(lang), used->file, used->index, merged ? " + " : "",
         merged ? merged->file : L"");
}

// Width of a combo whose widest entry is textW: text + arrow button + padding.
static float ComboWidth(float textW) {
    return textW + ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2.0f + 4.0f;
}

// Language list at the bottom of the panel (its popup opens upwards: our window ends right below it).
// Picking a language takes effect from the next frame: OverlayRun reloads the font and the wording.
static void LanguageCombo() {
    ImGui::TextUnformatted(T("lang"));
    ImGui::SameLine();
    int cur = g_opt.lang.load();
    float w = 0;
    for (int l = 0; l < kLangCount; ++l) w = (std::max)(w, ImGui::CalcTextSize(LangName(l)).x);
    ImGui::SetNextItemWidth(ComboWidth(w));
    if (ImGui::BeginCombo("##lang", LangName(cur))) {
        for (int l = 0; l < kLangCount; ++l) {
            if (ImGui::Selectable(LangName(l), l == cur) && l != cur) { g_opt.lang = l; g_dirty = true; }
            if (l == cur) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

// The panel never activates on click (WS_EX_NOACTIVATE), so a text box has to ask for the keyboard while it is
// being edited and hand it back to the game afterwards.  Allowed: our process owns the foreground window.
static void TakeKeyboard() {
    if (GetForegroundWindow() != g_hwnd) SetForegroundWindow(g_hwnd);
}
static void ReturnKeyboard() {
    if (GetForegroundWindow() == g_hwnd && g_gameHwnd && IsWindow(g_gameHwnd)) SetForegroundWindow(g_gameHwnd);
}

// Checkbox whose wording comes from the text file; the "##key" suffix keeps the ImGui id stable when the
// user edits the text (two equal labels would otherwise clash).
static bool Check(const char* key, std::atomic<bool>& v) {
    char label[512];
    snprintf(label, sizeof(label), "%s##%s", T(key), key);
    bool b = v.load();
    if (ImGui::Checkbox(label, &b)) { v = b; g_dirty = true; return true; }
    return false;
}

// Title line doubling as the drag handle (the window is borderless).
static void TitleBar() {
    static POINT s_from;
    static RECT s_win;
    char title[256];
    snprintf(title, sizeof(title), "%s  v%s", T("title"), TF3SNAP_VERSION);
    const ImVec2 at = ImGui::GetCursorScreenPos();
    const float h = ImGui::GetTextLineHeightWithSpacing();
    ImGui::InvisibleButton("##drag", ImVec2((std::max)(ImGui::CalcTextSize(title).x, 200.0f), h));
    if (ImGui::IsItemActivated()) { GetCursorPos(&s_from); GetWindowRect(g_hwnd, &s_win); }
    if (ImGui::IsItemActive()) {
        POINT c;
        GetCursorPos(&c);
        SetWindowPos(g_hwnd, nullptr, s_win.left + c.x - s_from.x, s_win.top + c.y - s_from.y, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (ImGui::IsItemDeactivated()) {
        RECT r;
        GetWindowRect(g_hwnd, &r);
        g_opt.panelX = r.left; g_opt.panelY = r.top; g_dirty = true;
    }
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    ImGui::GetWindowDrawList()->AddText(at, ImGui::GetColorU32(ImGuiCol_TextDisabled), title);
}

static void DrawPanel() {
    // The ImGui window fills our Win32 window, and the Win32 window is sized from the content (end of this
    // function).  Not AlwaysAutoResize: ImGui caps an auto-fit window at the display size, which here IS our
    // window, so after one small first frame it could never grow back (0.2.0's "black strip").
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("tf3snap", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

    TitleBar();
    if (!g_opt.hooksOk.load())
        ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", T("status.nohooks"));

    // ---- 貼齊：游標會不會被吸過去 ----
    ImGui::SeparatorText(T("sec.snap"));
    Check("snap.existing", g_opt.existing);
    Check("snap.length", g_opt.length);
    ImGui::SameLine();
    ImGui::TextUnformatted(T("snap.length.every"));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    float step = g_opt.lengthStep.load();
    if (ImGui::InputFloat("##step", &step, 0, 0, "%.0f")) {
        if (step < 1) step = 1;
        g_opt.lengthStep = step; g_dirty = true;
    }
    if (ImGui::IsItemActivated()) TakeKeyboard();
    if (ImGui::IsItemDeactivated()) ReturnKeyboard();   // Enter, or a click elsewhere
    ImGui::SameLine();
    ImGui::TextUnformatted(T("snap.length.unit"));
    Check("snap.sides", g_opt.sides);
    Check("snap.lanes", g_opt.lanes);
    Check("snap.guides", g_opt.snapGuides);
    Check("snap.nearby", g_opt.nearby);
    ImGui::TextUnformatted(T("snap.scale"));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180);
    float sc = g_opt.scale.load();
    if (ImGui::SliderFloat("##scale", &sc, 0.25f, 4.0f, "x%.2f")) { g_opt.scale = sc; g_dirty = true; }

    // ---- 引導線：畫面上看得到哪些；取消某一種 = 既不顯示也不吸附 ----
    ImGui::SeparatorText(T("sec.guide"));
    Check("guide.show", g_opt.showGuides);
    ImGui::Indent(22);
    Check("guide.angle", g_opt.guideAngle);
    ImGui::SameLine();
    const char* items[] = {T("guide.grid.default"), "90°", "45°", "30°", "15°"};
    const int vals[] = {0, 90, 45, 30, 15};
    int cur = 0;
    float itemsW = 0;   // fit the longest entry ("Game default" is wider than "遊戲預設")
    for (int i = 0; i < 5; ++i) {
        if (vals[i] == g_opt.grid.load()) cur = i;
        itemsW = (std::max)(itemsW, ImGui::CalcTextSize(items[i]).x);
    }
    ImGui::SetNextItemWidth(ComboWidth(itemsW));
    if (!g_opt.guideAngle.load()) ImGui::BeginDisabled();
    if (ImGui::Combo("##grid", &cur, items, 5)) { g_opt.grid = vals[cur]; g_dirty = true; }
    if (!g_opt.guideAngle.load()) ImGui::EndDisabled();
    Check("guide.straight", g_opt.guideStraight);
    if (!g_opt.crossOk.load()) ImGui::BeginDisabled();
    Check("guide.cross", g_opt.guideCross);
    if (!g_opt.crossOk.load()) ImGui::EndDisabled();
    ImGui::Unindent(22);
    if (g_opt.hooksOk.load() && !g_opt.crossOk.load())
        ImGui::TextColored(ImVec4(1, 0.6f, 0.3f, 1), "%s", T("status.nocross"));

    ImGui::Separator();
    char b1[256], b2[256], b3[256];
    snprintf(b1, sizeof(b1), "%s##allOn", T("btn.allOn"));
    snprintf(b2, sizeof(b2), "%s##allOff", T("btn.allOff"));
    snprintf(b3, sizeof(b3), "%s##reset", T("btn.reset"));
    auto setAll = [](bool v) {
        g_opt.existing = g_opt.sides = g_opt.lanes = g_opt.snapGuides = g_opt.nearby = v;
        g_opt.showGuides = g_opt.guideAngle = g_opt.guideStraight = g_opt.guideCross = v;
        g_dirty = true;
    };
    if (ImGui::Button(b1)) setAll(true);
    ImGui::SameLine();
    if (ImGui::Button(b2)) setAll(false);
    ImGui::SameLine();
    if (ImGui::Button(b3)) { ResetToStock(); g_dirty = true; }

    const bool canDetect = g_opt.toolDetect.load();
    if (!canDetect) ImGui::BeginDisabled();
    Check("toolOnly", g_opt.toolOnly);
    if (!canDetect) ImGui::EndDisabled();
    if (g_opt.hooksOk.load() && !canDetect)
        ImGui::TextColored(ImVec4(1, 0.6f, 0.3f, 1), "%s", T("status.notool"));
    LanguageCombo();

    // The Win32 window follows the content (wording is user-editable, so the size changes).  ContentSizeIdeal
    // is the content extent measured last frame and is NOT capped by the window size; every item here has a
    // fixed or text-derived width, so it does not depend on the window width either -> no feedback loop.
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImVec2 pad = ImGui::GetStyle().WindowPadding;
    const float wantW = win->ContentSizeIdeal.x + pad.x * 2.0f + 2.0f;
    const float wantH = win->ContentSizeIdeal.y + pad.y * 2.0f + 2.0f;
    ImGui::End();
    if (wantW >= 120.0f && wantH >= 80.0f) {   // the very first frame has measured nothing yet
        const int w = (std::min)((int)(wantW + 0.5f), 1600), h = (std::min)((int)(wantH + 0.5f), 1400);
        RECT rc;
        GetClientRect(g_hwnd, &rc);
        if (w != rc.right - rc.left || h != rc.bottom - rc.top)
            SetWindowPos(g_hwnd, nullptr, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

// The road / track tool counts as open while StreetBuilder::Step ran within this long; it runs every frame,
// so this only rides over load hitches and is how long the panel lingers after the tool is closed.
static constexpr uint64_t kToolGraceMs = 500;

static bool CursorOverPanel() {
    POINT c;
    RECT r;
    return GetCursorPos(&c) && GetWindowRect(g_hwnd, &r) && PtInRect(&r, c);
}

// Why the panel should be hidden (it goes to the log), or nullptr when it should show.  The panel follows the
// road / track tool and nothing else of the game's state: no ESC / pause test (0.3.1's hid it whenever paused).
static const char* HideReason(bool shown) {
    // A top-most panel must not cover other programs: only while the game window or the panel is in front.
    DWORD fgpid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &fgpid);
    if (fgpid != GetCurrentProcessId()) return "game not in front";
    if (g_opt.toolOnly.load() && g_opt.toolDetect.load()) {
        if (BuilderToolOpen(kToolGraceMs)) return nullptr;
        // Never vanish under the user's hand: a visible panel stays while it has focus (it was clicked, and
        // the game may step less while it is in the background) or the mouse is on it.
        if (shown && (GetForegroundWindow() == g_hwnd || CursorOverPanel())) return nullptr;
        return "road / track tool closed";
    }
    return InGame() ? nullptr : "not in a game";   // tool check off or unavailable: whenever a save is loaded
}

void OverlayRun() {
    WNDCLASSEXW wc = {sizeof(wc)};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"TF3SnapOverlay";
    wc.hCursor = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));
    RegisterClassExW(&wc);

    // last position, kept on screen (a monitor may have gone away since)
    int x = g_opt.panelX.load(), y = g_opt.panelY.load();
    const int vx = GetSystemMetrics(SM_XVIRTUALSCREEN), vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN), vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (x < vx || y < vy || x > vx + vw - 100 || y > vy + vh - 60) { x = 40; y = 120; }
    g_hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, wc.lpszClassName, L"Better Snap Option",
                             WS_POPUP, x, y, 420, 560, nullptr, nullptr, wc.hInstance, nullptr);
    if (!g_hwnd) { Logf("overlay: CreateWindow failed"); return; }
    if (!CreateDevice(g_hwnd)) { Logf("overlay: D3D11 device failed"); DestroyWindow(g_hwnd); return; }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;   // don't scatter imgui.ini
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    int fontLang = g_opt.lang.load();
    LoadFont(fontLang);
    TextInit();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);
    Logf("overlay: up");

    // Saving: a dragged slider changes an option every frame and the game folder may be on a slow (here: USB)
    // drive, so write once the changes have paused, when the panel hides, and on the way out.
    constexpr uint64_t kSaveDelayMs = 800;
    // Redrawing: at the display rate while the mouse is on the panel, a widget is held or a text box is being
    // edited; otherwise when a message arrives or every kIdleMs (nothing on the panel moves by itself).
    constexpr DWORD kIdleMs = 100;
    bool shown = false, busy = false, savePending = false;
    uint64_t saveAt = 0, lastTextPoll = 0;
    bool toolWasOpen = false;
    while (!g_stop) {
        if (shown && !busy) MsgWaitForMultipleObjectsEx(0, nullptr, kIdleMs, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (g_stop) break;

        const uint64_t now = GetTickCount64();
        const int lang = g_opt.lang.load();
        if (lang != fontLang || now - lastTextPoll > 1000) { lastTextPoll = now; TextPoll(); }
        if (lang != fontLang) { LoadFont(lang); fontLang = lang; }   // between frames, as ClearFonts wants

        // tool open / closed transitions go to the log, so a play session shows the detection worked
        const bool toolOpen = g_opt.toolDetect.load() && BuilderToolOpen(kToolGraceMs);
        if (toolOpen != toolWasOpen) {
            Logf("road / track tool %s", toolOpen ? "opened" : "closed");
            toolWasOpen = toolOpen;
        }

        // the game's own window, to hand the keyboard back to after a text box was edited
        const HWND fg = GetForegroundWindow();
        DWORD fgpid = 0;
        GetWindowThreadProcessId(fg, &fgpid);
        if (fg && fg != g_hwnd && fgpid == GetCurrentProcessId()) g_gameHwnd = fg;

        if (g_dirty) { g_dirty = false; savePending = true; saveAt = now + kSaveDelayMs; }

        const char* hideWhy = HideReason(shown);
        const bool show = !hideWhy;
        if (show != shown) {
            ShowWindow(g_hwnd, show ? SW_SHOWNOACTIVATE : SW_HIDE);
            shown = show;
            if (show) Logf("panel shown");
            else Logf("panel hidden: %s", hideWhy);
        }
        if (savePending && (!show || now >= saveAt)) { SaveConfig(); savePending = false; }
        if (!show || !g_rtv) { Sleep(60); continue; }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        DrawPanel();
        ImGui::Render();
        const float clear[4] = {0.07f, 0.07f, 0.08f, 1.0f};
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(1, 0);

        busy = CursorOverPanel() || ImGui::IsAnyItemActive() || GetForegroundWindow() == g_hwnd;
    }
    if (savePending || g_dirty) SaveConfig();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    if (g_hwnd) { DestroyWindow(g_hwnd); g_hwnd = nullptr; }
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    Logf("overlay: down");
}

void OverlayStop() { g_stop = true; }

}  // namespace tf3
