// Minimal logging to tf3snap\tf3snap.log (and OutputDebugString).  No dependency beyond Win32/CRT.
//
// tf3snap.log is always the current game session.  At start-up the previous one moves to
// tf3snap\logs\tf3snap_<session start>.log and only the newest kKeepOld of those stay, so the last 5 sessions
// are on disk: a session that crashed survives four more launches.  The first line of every log carries the
// session's date ("=== Better Snap Option session 2026-10-16 00:06:12 ===", "TF3 Snap Options" in 0.2.2),
// because the lines themselves only have the time of day; a log without it (0.2.1 and older) is named after
// its last write instead.
#include "tf3snap.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace tf3 {

// Defined in config.cpp; the plugin's own folder (...\tf3snap\).
std::wstring PluginDir();

static constexpr size_t kKeepOld = 4;   // + the current tf3snap.log = the last 5 sessions
static constexpr char kHeader[] = "=== " TF3SNAP_PRODUCT " session ";
static constexpr char kOldHeader[] = "=== TF3 Snap Options session ";   // 0.2.2, before the rename

static std::mutex g_logMx;
static std::wstring g_logPath;

static std::wstring Stamp(const SYSTEMTIME& t) {
    wchar_t s[32];
    swprintf(s, 32, L"%04u-%02u-%02u_%02u%02u%02u", (unsigned)t.wYear, (unsigned)t.wMonth, (unsigned)t.wDay,
             (unsigned)t.wHour, (unsigned)t.wMinute, (unsigned)t.wSecond);
    return s;
}

// When the session in this log started: from its header line, else (older logs) from its last write.
static std::wstring SessionStamp(const std::wstring& path) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        char buf[96] = {};
        DWORD n = 0;
        ReadFile(h, buf, sizeof(buf) - 1, &n, nullptr);
        CloseHandle(h);
        unsigned y, mo, d, hh, mi, ss;
        const char* date = strncmp(buf, kHeader, sizeof(kHeader) - 1) == 0 ? buf + sizeof(kHeader) - 1
                         : strncmp(buf, kOldHeader, sizeof(kOldHeader) - 1) == 0 ? buf + sizeof(kOldHeader) - 1
                         : nullptr;
        if (date && sscanf(date, "%u-%u-%u %u:%u:%u", &y, &mo, &d, &hh, &mi, &ss) == 6) {
            SYSTEMTIME t = {};
            t.wYear = (WORD)y; t.wMonth = (WORD)mo; t.wDay = (WORD)d;
            t.wHour = (WORD)hh; t.wMinute = (WORD)mi; t.wSecond = (WORD)ss;
            return Stamp(t);
        }
    }
    WIN32_FILE_ATTRIBUTE_DATA a;
    SYSTEMTIME utc, local;
    if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &a) && FileTimeToSystemTime(&a.ftLastWriteTime, &utc) &&
        SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local))
        return Stamp(local);
    GetLocalTime(&local);
    return Stamp(local);
}

// Move a finished session's log into logs\ under its start time.  Returns the new name ("" if it failed).
static std::wstring Archive(const std::wstring& path, const std::wstring& dir) {
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) return L"";
    const std::wstring base = dir + L"tf3snap_" + SessionStamp(path);
    std::wstring to = base + L".log";
    for (int i = 2; GetFileAttributesW(to.c_str()) != INVALID_FILE_ATTRIBUTES && i < 100; ++i)
        to = base + L"-" + std::to_wstring(i) + L".log";
    return MoveFileW(path.c_str(), to.c_str()) ? to : L"";
}

// Delete all but the newest kKeepOld archived logs (the names sort by time).
static int Prune(const std::wstring& dir) {
    std::vector<std::wstring> names;
    WIN32_FIND_DATAW fd;
    HANDLE f = FindFirstFileW((dir + L"tf3snap_*.log").c_str(), &fd);
    if (f == INVALID_HANDLE_VALUE) return 0;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) names.push_back(fd.cFileName);
    } while (FindNextFileW(f, &fd));
    FindClose(f);
    if (names.size() <= kKeepOld) return 0;
    std::sort(names.begin(), names.end(), [](const std::wstring& a, const std::wstring& b) { return a > b; });
    int deleted = 0;
    for (size_t i = kKeepOld; i < names.size(); ++i)
        if (DeleteFileW((dir + names[i]).c_str())) ++deleted;
    return deleted;
}

void LogInit() {
    std::vector<std::wstring> archived;
    int pruned = 0;
    {
        std::lock_guard<std::mutex> lk(g_logMx);
        const std::wstring dir = PluginDir();
        const std::wstring logs = dir + L"logs\\";
        CreateDirectoryW(logs.c_str(), nullptr);   // fails harmlessly when it exists
        // 0.2.1 and older kept one previous session as tf3snap.prev.log: keep it in the history as well
        for (const wchar_t* name : {L"tf3snap.prev.log", L"tf3snap.log"}) {
            std::wstring to = Archive(dir + name, logs);
            if (!to.empty()) archived.push_back(to.substr(dir.size()));
        }
        pruned = Prune(logs);

        g_logPath = dir + L"tf3snap.log";
        HANDLE h = CreateFileW(g_logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            SYSTEMTIME t;
            GetLocalTime(&t);
            char head[96];
            const int n = snprintf(head, sizeof(head), "%s%04u-%02u-%02u %02u:%02u:%02u ===\r\n", kHeader, (unsigned)t.wYear,
                                   (unsigned)t.wMonth, (unsigned)t.wDay, (unsigned)t.wHour,
                                   (unsigned)t.wMinute, (unsigned)t.wSecond);
            DWORD w = 0;
            if (n > 0) WriteFile(h, head, (DWORD)n, &w, nullptr);
            CloseHandle(h);
        }
    }
    for (const auto& a : archived) Logf("log: previous session kept as %ls", a.c_str());
    if (pruned) Logf("log: removed %d old session log(s), keeping the newest %d", pruned, (int)kKeepOld);
}

void Logf(const char* fmt, ...) {
    char msg[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(msg, sizeof(msg) - 2, fmt, ap);
    va_end(ap);
    if (n < 0) return;

    SYSTEMTIME st;
    GetLocalTime(&st);
    char line[1152];
    int m = snprintf(line, sizeof(line), "[%02d:%02d:%02d.%03d] %s\r\n",
                     st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, msg);
    if (m < 0) return;

    OutputDebugStringA(line);
    std::lock_guard<std::mutex> lk(g_logMx);
    if (g_logPath.empty()) return;
    HANDLE h = CreateFileW(g_logPath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    SetFilePointer(h, 0, nullptr, FILE_END);
    DWORD written = 0;
    WriteFile(h, line, (DWORD)m, &written, nullptr);
    CloseHandle(h);
}

}  // namespace tf3
