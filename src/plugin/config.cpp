// Plugin folder resolution + tf3snap.ini load/save (maps 1:1 to tf3::Options).
#include "tf3snap.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace tf3 {

// Directory containing this DLL (ends with a backslash).  Found via this function's own address so it
// works no matter what the host or current directory is.
std::wstring PluginDir() {
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&PluginDir), &self);
    wchar_t path[MAX_PATH];
    DWORD n = GetModuleFileNameW(self, path, MAX_PATH);
    while (n > 0 && path[n - 1] != L'\\' && path[n - 1] != L'/') --n;
    path[n] = 0;
    return std::wstring(path);
}

std::wstring ConfigPath() { return PluginDir() + L"tf3snap.ini"; }

static std::string ReadAll(const std::wstring& path) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return {};
    std::string out;
    char buf[4096];
    DWORD r = 0;
    while (ReadFile(h, buf, sizeof(buf), &r, nullptr) && r) out.append(buf, r);
    CloseHandle(h);
    return out;
}

void LoadConfig() {
    std::string s = ReadAll(ConfigPath());
    if (s.empty()) {
        g_opt.lang = LangFromSystem();   // first run: follow the Windows display language
        return;
    }
    bool sawGuideAngle = false, oldAngle90 = false;
    g_opt.lang = kLangZhTW;   // an ini from 0.2.1 or older has no lang= : those panels were Traditional Chinese
    size_t i = 0;
    while (i < s.size()) {
        size_t nl = s.find('\n', i);
        if (nl == std::string::npos) nl = s.size();
        std::string line = s.substr(i, nl - i);
        i = nl + 1;
        size_t eq = line.find('=');
        if (eq == std::string::npos || line.empty() || line[0] == '#' || line[0] == ';') continue;
        std::string k = line.substr(0, eq);
        std::string v = line.substr(eq + 1);
        auto trim = [](std::string& x) {
            while (!x.empty() && (x.back() == '\r' || x.back() == ' ' || x.back() == '\t')) x.pop_back();
            size_t p = 0; while (p < x.size() && (x[p] == ' ' || x[p] == '\t')) ++p; x.erase(0, p);
        };
        trim(k); trim(v);
        bool bv = (v == "1" || v == "true" || v == "True");
        if (k == "existing") g_opt.existing = bv;
        else if (k == "length") g_opt.length = bv;
        else if (k == "sides") g_opt.sides = bv;
        else if (k == "lanes") g_opt.lanes = bv;
        else if (k == "snapGuides" || k == "guides") g_opt.snapGuides = bv;   // "guides": 0.1.x name
        else if (k == "nearby") g_opt.nearby = bv;
        else if (k == "showGuides") g_opt.showGuides = bv;
        else if (k == "guideAngle") { g_opt.guideAngle = bv; sawGuideAngle = true; }
        else if (k == "guideStraight") g_opt.guideStraight = bv;
        else if (k == "guideCross") g_opt.guideCross = bv;
        else if (k == "angle90") oldAngle90 = bv;                             // 0.1.x: now the 90 deg grid
        else if (k == "toolOnly") g_opt.toolOnly = bv;   // (0.2.0's "autohide" is always on now: ignored)
        else if (k == "lang") { const int l = LangFromCode(v); if (l >= 0) g_opt.lang = l; }
        else if (k == "lengthStep") g_opt.lengthStep = (float)atof(v.c_str());
        else if (k == "scale") g_opt.scale = (float)atof(v.c_str());
        else if (k == "grid") g_opt.grid = atoi(v.c_str());
        else if (k == "panelX") g_opt.panelX = atoi(v.c_str());
        else if (k == "panelY") g_opt.panelY = atoi(v.c_str());
    }
    if (oldAngle90 && !sawGuideAngle) g_opt.grid = 90;
    Logf("config loaded from %ls", ConfigPath().c_str());
}

void SaveConfig() {
    char buf[1024];
    int n = snprintf(buf, sizeof(buf),
                     "# " TF3SNAP_PRODUCT "\r\n"
                     "existing=%d\r\nlength=%d\r\nlengthStep=%.3g\r\nsides=%d\r\nlanes=%d\r\n"
                     "snapGuides=%d\r\nnearby=%d\r\nscale=%.3g\r\n"
                     "showGuides=%d\r\nguideAngle=%d\r\ngrid=%d\r\nguideStraight=%d\r\nguideCross=%d\r\n"
                     "toolOnly=%d\r\nlang=%s\r\npanelX=%d\r\npanelY=%d\r\n",
                     (int)g_opt.existing.load(), (int)g_opt.length.load(), g_opt.lengthStep.load(),
                     (int)g_opt.sides.load(), (int)g_opt.lanes.load(), (int)g_opt.snapGuides.load(),
                     (int)g_opt.nearby.load(), g_opt.scale.load(), (int)g_opt.showGuides.load(),
                     (int)g_opt.guideAngle.load(), g_opt.grid.load(), (int)g_opt.guideStraight.load(),
                     (int)g_opt.guideCross.load(), (int)g_opt.toolOnly.load(), LangCode(g_opt.lang.load()),
                     g_opt.panelX.load(), g_opt.panelY.load());
    if (n <= 0) return;
    HANDLE h = CreateFileW(ConfigPath().c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD w = 0;
    WriteFile(h, buf, (DWORD)n, &w, nullptr);
    CloseHandle(h);
}

void ResetToStock() {
    g_opt.existing = true; g_opt.length = false; g_opt.lengthStep = 8.0f;
    g_opt.sides = true; g_opt.lanes = true; g_opt.snapGuides = true; g_opt.nearby = true; g_opt.scale = 1.0f;
    g_opt.showGuides = true; g_opt.guideAngle = true; g_opt.grid = 0; g_opt.guideStraight = true;
    g_opt.guideCross = false;   // ours, not the game's
}

}  // namespace tf3
