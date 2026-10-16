// Panel wording, editable without recompiling: one file per language, tf3snap\tf3snap_text_<code>.ini
// (UTF-8, "key = text" per line).  Missing files are written with the built-in text so there is a template to
// edit.  Missing / blank keys fall back to the built-in text of that language.  The panel's language list picks
// the file; the overlay calls TextPoll() about once a second (and right after a switch) and reloads on save.
// Only the overlay thread touches this, so there is no locking.
#include "tf3snap.h"

#include <cstring>
#include <map>
#include <string>

namespace tf3 {

std::wstring PluginDir();

struct DefaultText { const char* key; const char* text; };

static const DefaultText kZhTW[] = {
    {"title", "更好的吸附貼齊"},
    {"status.nohooks", "未掛鉤：遊戲版本與特徵碼不符（遊戲未被改動）"},
    {"status.nocross", "橫向導引線無法使用：這個遊戲版本找不到需要的函式"},
    {"status.notool", "認不出道路 / 鐵路工具（這個遊戲版本找不到函式）：面板會一直顯示"},
    {"sec.snap", "貼齊"},
    {"snap.existing", "1  貼齊既有道路 / 軌道"},
    {"snap.length", "2  貼齊長度"},
    {"snap.length.every", "每"},
    {"snap.length.unit", "公尺"},
    {"snap.sides", "3  貼齊道路側邊（並排）"},
    {"snap.lanes", "4  貼齊車道（與既有道路重疊的車道偏移）"},
    {"snap.guides", "5  貼齊引導線"},
    {"snap.nearby", "6  貼齊鄰近路口節點"},
    {"snap.scale", "貼齊範圍"},
    {"sec.guide", "引導線"},
    {"guide.show", "顯示引導線"},
    {"guide.angle", "角度導引線"},
    {"guide.grid.default", "遊戲預設"},
    {"guide.straight", "縱向導引線（沿既有道路方向延伸）"},
    {"guide.cross", "橫向導引線（對齊附近道路的端點）"},
    {"btn.allOn", "全部開"},
    {"btn.allOff", "全部關"},
    {"btn.reset", "恢復遊戲預設"},
    {"toolOnly", "只在道路 / 鐵路工具開啟時顯示"},
    {"lang", "語言"},
};

static const DefaultText kZhCN[] = {
    {"title", "更好的吸附贴齐"},
    {"status.nohooks", "未安装钩子：游戏版本与特征码不匹配（未改动游戏）"},
    {"status.nocross", "横向辅助线不可用：此游戏版本中找不到所需函数"},
    {"status.notool", "无法识别道路 / 铁路工具（此游戏版本中找不到相应函数）：面板将始终显示"},
    {"sec.snap", "吸附"},
    {"snap.existing", "1  吸附到已有道路 / 轨道"},
    {"snap.length", "2  按长度吸附"},
    {"snap.length.every", "每"},
    {"snap.length.unit", "米"},
    {"snap.sides", "3  吸附道路边缘（并排）"},
    {"snap.lanes", "4  吸附车道（与已有道路重叠的车道偏移）"},
    {"snap.guides", "5  吸附辅助线"},
    {"snap.nearby", "6  吸附附近路口节点"},
    {"snap.scale", "吸附范围"},
    {"sec.guide", "辅助线"},
    {"guide.show", "显示辅助线"},
    {"guide.angle", "角度辅助线"},
    {"guide.grid.default", "游戏默认"},
    {"guide.straight", "纵向辅助线（沿已有道路方向延伸）"},
    {"guide.cross", "横向辅助线（对齐附近道路的端点）"},
    {"btn.allOn", "全部开启"},
    {"btn.allOff", "全部关闭"},
    {"btn.reset", "恢复游戏默认"},
    {"toolOnly", "仅在道路 / 铁路工具打开时显示"},
    {"lang", "语言"},
};

static const DefaultText kEn[] = {
    {"title", "Better Snap Option"},
    {"status.nohooks", "Not hooked: the game version does not match the signatures (game left unchanged)"},
    {"status.nocross", "Cross guides unavailable: this game version lacks the functions they need"},
    {"status.notool", "Cannot detect the road / track tool (function not found in this game version): the panel stays visible"},
    {"sec.snap", "Snapping"},
    {"snap.existing", "1  Snap to existing roads / tracks"},
    {"snap.length", "2  Snap length"},
    {"snap.length.every", "every"},
    {"snap.length.unit", "m"},
    {"snap.sides", "3  Snap to road edges (side by side)"},
    {"snap.lanes", "4  Snap to lanes (lane offsets overlapping existing roads)"},
    {"snap.guides", "5  Snap to guides"},
    {"snap.nearby", "6  Snap to nearby junction nodes"},
    {"snap.scale", "Snap range"},
    {"sec.guide", "Guides"},
    {"guide.show", "Show guides"},
    {"guide.angle", "Angle guides"},
    {"guide.grid.default", "Game default"},
    {"guide.straight", "Straight guides (extend along existing roads)"},
    {"guide.cross", "Cross guides (line up with nearby road ends)"},
    {"btn.allOn", "All on"},
    {"btn.allOff", "All off"},
    {"btn.reset", "Reset to game defaults"},
    {"toolOnly", "Show only while the road / track tool is open"},
    {"lang", "Language"},
};

static const DefaultText kJa[] = {
    {"title", "Better Snap Option"},
    {"status.nohooks", "フック未適用：ゲームのバージョンがシグネチャと一致しません（ゲームは変更されていません）"},
    {"status.nocross", "横方向ガイドは使用できません：このバージョンのゲームに必要な関数が見つかりません"},
    {"status.notool", "道路 / 線路ツールを検出できません（このバージョンのゲームに関数が見つかりません）：パネルは常に表示されます"},
    {"sec.snap", "スナップ"},
    {"snap.existing", "1  既存の道路 / 線路にスナップ"},
    {"snap.length", "2  長さにスナップ"},
    {"snap.length.every", "間隔"},
    {"snap.length.unit", "m"},
    {"snap.sides", "3  道路の側端にスナップ（並列）"},
    {"snap.lanes", "4  車線にスナップ（既存道路と重なる車線オフセット）"},
    {"snap.guides", "5  ガイドにスナップ"},
    {"snap.nearby", "6  近くの交差点ノードにスナップ"},
    {"snap.scale", "スナップ範囲"},
    {"sec.guide", "ガイド"},
    {"guide.show", "ガイドを表示"},
    {"guide.angle", "角度ガイド"},
    {"guide.grid.default", "ゲーム既定"},
    {"guide.straight", "縦方向ガイド（既存道路の方向に延長）"},
    {"guide.cross", "横方向ガイド（近くの道路の端点に揃える）"},
    {"btn.allOn", "すべてオン"},
    {"btn.allOff", "すべてオフ"},
    {"btn.reset", "ゲーム既定に戻す"},
    {"toolOnly", "道路 / 線路ツールを開いている時のみ表示"},
    {"lang", "言語"},
};

struct Language {
    const char* code;     // file suffix and tf3snap.ini value
    const char* name;     // shown in the language list, in its own language
    const DefaultText* text;
    size_t count;
    const char* help;     // template header lines in that language
};
#define TF3_LANG(code, name, table, help) {code, name, table, sizeof(table) / sizeof(table[0]), help}
static const Language kLangs[kLangCount] = {
    TF3_LANG("zh-TW", "繁體中文", kZhTW,
             "# 面板文字：一行一個「代號 = 顯示文字」，只改等號右邊。用 UTF-8 存檔，面板一秒內更新。\r\n"
             "# 刪掉或留空的行會用內建文字。要換行就寫 \\n。\r\n"),
    TF3_LANG("zh-CN", "简体中文", kZhCN,
             "# 面板文字：每行一个“代号 = 显示文字”，只改等号右边。用 UTF-8 保存，面板一秒内更新。\r\n"
             "# 删除或留空的行会使用内置文字。要换行就写 \\n。\r\n"),
    TF3_LANG("en", "English", kEn, ""),
    TF3_LANG("ja", "日本語", kJa,
             "# パネルの文字：1 行に 1 つ「キー = 表示文字」。等号の右側だけを変更し、UTF-8 で保存すると 1 秒以内に反映されます。\r\n"
             "# 削除した行や空の行は内蔵の文字に戻ります。改行は \\n と書きます。\r\n"),
};
#undef TF3_LANG

static int Clamp(int lang) { return lang >= 0 && lang < kLangCount ? lang : kLangZhTW; }

const char* LangCode(int lang) { return kLangs[Clamp(lang)].code; }
const char* LangName(int lang) { return kLangs[Clamp(lang)].name; }

int LangFromCode(const std::string& code) {
    for (int i = 0; i < kLangCount; ++i)
        if (_stricmp(code.c_str(), kLangs[i].code) == 0) return i;
    return -1;
}

int LangFromSystem() {
    const LANGID id = GetUserDefaultUILanguage();
    switch (PRIMARYLANGID(id)) {
        case LANG_CHINESE:
            // Simplified: PRC, Singapore.  Everything else (Taiwan, Hong Kong, Macao) is Traditional.
            return (SUBLANGID(id) == SUBLANG_CHINESE_SIMPLIFIED || SUBLANGID(id) == SUBLANG_CHINESE_SINGAPORE)
                       ? kLangZhCN : kLangZhTW;
        case LANG_JAPANESE: return kLangJa;
        default: return kLangEn;
    }
}

static std::map<std::string, std::string> g_text;   // overrides loaded from the current language's file
static int g_loaded = -1;                            // language g_text belongs to
static FILETIME g_stamp = {};
static bool g_haveStamp = false;

static std::wstring TextPath(int lang) {
    const char* code = LangCode(lang);
    return PluginDir() + L"tf3snap_text_" + std::wstring(code, code + strlen(code)) + L".ini";
}

static std::string ReadFileUtf8(const std::wstring& path, bool* hadBom = nullptr) {
    if (hadBom) *hadBom = false;
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return {};
    std::string out;
    char buf[4096];
    DWORD r = 0;
    while (ReadFile(h, buf, sizeof(buf), &r, nullptr) && r) out.append(buf, r);
    CloseHandle(h);
    if (out.size() >= 3 && (unsigned char)out[0] == 0xEF && (unsigned char)out[1] == 0xBB &&
        (unsigned char)out[2] == 0xBF) {
        out.erase(0, 3);   // Notepad's UTF-8 BOM
        if (hadBom) *hadBom = true;
    }
    return out;
}

static void Trim(std::string& x) {
    while (!x.empty() && (x.back() == '\r' || x.back() == ' ' || x.back() == '\t')) x.pop_back();
    size_t p = 0;
    while (p < x.size() && (x[p] == ' ' || x[p] == '\t')) ++p;
    x.erase(0, p);
}

static void WriteTemplate(int lang) {
    const Language& L = kLangs[lang];
    std::string s = std::string("# " TF3SNAP_PRODUCT " - panel text, ") + L.name + " (" + L.code + ").\r\n" +
        "# One per line:  key = text.  Only change the part after '='.  Save as UTF-8; the panel updates\r\n"
        "# within a second.  A deleted or empty line falls back to the built-in text.  Write \\n for a line break.\r\n"
        "# One file per language; the language list at the bottom of the panel picks which one is used.\r\n"
        "#\r\n" + L.help + "\r\n";
    for (size_t i = 0; i < L.count; ++i) {
        s += L.text[i].key;
        s += " = ";
        s += L.text[i].text;
        s += "\r\n";
    }
    HANDLE h = CreateFileW(TextPath(lang).c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD w = 0;
    WriteFile(h, s.data(), (DWORD)s.size(), &w, nullptr);
    CloseHandle(h);
    Logf("text: wrote template %ls", TextPath(lang).c_str());
}

static void Load(int lang) {
    std::string s = ReadFileUtf8(TextPath(lang));
    std::map<std::string, std::string> m;
    size_t i = 0;
    while (i < s.size()) {
        size_t nl = s.find('\n', i);
        if (nl == std::string::npos) nl = s.size();
        std::string line = s.substr(i, nl - i);
        i = nl + 1;
        Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        Trim(k);
        Trim(v);
        for (size_t p = 0; (p = v.find("\\n", p)) != std::string::npos;) v.replace(p, 2, "\n");
        if (!k.empty() && !v.empty()) m[k] = v;
    }
    g_text.swap(m);
    g_loaded = lang;
}

// Built-in texts of earlier versions.  A text file line that still holds one came from the template and was
// never edited, so UpgradeFile swaps it for today's built-in text; lines the user changed are left alone.
struct RetiredText { int lang; const char* key; const char* text; };
static const RetiredText kRetired[] = {
    {kLangZhTW, "title", "TF3 貼齊選項"},   // 0.2.x, renamed in 0.3.0
    {kLangZhCN, "title", "TF3 吸附选项"},
    {kLangEn, "title", "TF3 Snap Options"},
    {kLangJa, "title", "TF3 スナップ設定"},
};
static constexpr char kOldHead[] = "# TF3 Snap Options - panel text";   // template header before 0.3.0
static constexpr char kNewHead[] = "# " TF3SNAP_PRODUCT " - panel text";

static const char* Builtin(int lang, const char* key);

static void UpgradeFile(int lang) {
    const std::wstring path = TextPath(lang);
    bool bom = false;
    const std::string s = ReadFileUtf8(path, &bom);
    if (s.empty()) return;
    std::string out;
    int changed = 0;
    size_t i = 0;
    while (i < s.size()) {
        size_t nl = s.find('\n', i);
        const size_t end = nl == std::string::npos ? s.size() : nl + 1;
        std::string raw = s.substr(i, end - i);
        i = end;
        std::string eol;
        while (!raw.empty() && (raw.back() == '\n' || raw.back() == '\r')) {
            eol.insert(eol.begin(), raw.back());
            raw.pop_back();
        }
        if (raw.compare(0, sizeof(kOldHead) - 1, kOldHead) == 0) {
            out += kNewHead + raw.substr(sizeof(kOldHead) - 1) + eol;
            ++changed;
            continue;
        }
        const size_t eq = raw.find('=');
        if (eq != std::string::npos && raw[0] != '#' && raw[0] != ';') {
            std::string k = raw.substr(0, eq), v = raw.substr(eq + 1);
            Trim(k);
            Trim(v);
            bool swapped = false;
            for (const RetiredText& r : kRetired)
                if (r.lang == lang && k == r.key && v == r.text) {
                    out += k + " = " + Builtin(lang, r.key) + eol;
                    swapped = true;
                    ++changed;
                    break;
                }
            if (swapped) continue;
        }
        out += raw + eol;
    }
    if (!changed) return;
    // write next to it, then swap in, so a failure never leaves half a file
    const std::wstring tmp = path + L".tmp";
    HANDLE h = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD w = 0;
    bool ok = true;
    if (bom) ok = WriteFile(h, "\xEF\xBB\xBF", 3, &w, nullptr) && w == 3;
    ok = ok && WriteFile(h, out.data(), (DWORD)out.size(), &w, nullptr) && w == out.size();
    CloseHandle(h);
    if (ok && MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        Logf("text: updated %d line(s) of an earlier version's built-in text in %ls", changed, path.c_str());
    else
        DeleteFileW(tmp.c_str());
}

static bool Stamp(int lang, FILETIME& ft) {
    WIN32_FILE_ATTRIBUTE_DATA a;
    if (!GetFileAttributesExW(TextPath(lang).c_str(), GetFileExInfoStandard, &a)) return false;
    ft = a.ftLastWriteTime;
    return true;
}

void TextInit() {
    // 0.2.1 and older had a single Traditional Chinese file; keep the user's edits as the zh-TW file.
    const std::wstring old = PluginDir() + L"tf3snap_text.ini";
    if (GetFileAttributesW(old.c_str()) != INVALID_FILE_ATTRIBUTES &&
        MoveFileW(old.c_str(), TextPath(kLangZhTW).c_str()))
        Logf("text: renamed %ls -> %ls", old.c_str(), TextPath(kLangZhTW).c_str());
    for (int i = 0; i < kLangCount; ++i)
        if (GetFileAttributesW(TextPath(i).c_str()) == INVALID_FILE_ATTRIBUTES) WriteTemplate(i);
        else UpgradeFile(i);
    TextPoll();
}

bool TextPoll() {
    const int lang = Clamp(g_opt.lang.load());
    FILETIME ft;
    bool have = Stamp(lang, ft);
    if (lang == g_loaded && have == g_haveStamp && (!have || CompareFileTime(&ft, &g_stamp) == 0)) return false;
    g_haveStamp = have;
    g_stamp = ft;
    Load(lang);
    Logf("text: %s, %d entries from %ls", LangCode(lang), (int)g_text.size(), TextPath(lang).c_str());
    return true;
}

static const char* Builtin(int lang, const char* key) {
    const Language& L = kLangs[Clamp(lang)];
    for (size_t i = 0; i < L.count; ++i)
        if (strcmp(L.text[i].key, key) == 0) return L.text[i].text;
    return nullptr;
}

const char* T(const char* key) {
    auto it = g_text.find(key);
    if (it != g_text.end()) return it->second.c_str();
    if (const char* d = Builtin(g_loaded, key)) return d;
    if (const char* d = Builtin(kLangZhTW, key)) return d;   // the reference table has every key
    return key;   // unknown key: show the key itself so a typo is visible
}

}  // namespace tf3
