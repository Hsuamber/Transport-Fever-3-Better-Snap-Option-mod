/* vulkan-1.dll proxy for Transport Fever 3.
 *
 * The game loads vulkan-1.dll by name; Windows looks in the game folder first, so this DLL is
 * loaded instead of the system Vulkan loader.  Every export is a jmp through a slot (exports.S)
 * that is filled here from the real loader in System32, so Vulkan keeps working unchanged.
 * Besides that it only loads tf3snap\tf3snap.dll, and only when the host is TransportFever3.exe.
 *
 * Disable without uninstalling: create an empty file tf3snap\disabled next to this DLL.
 */
#include <windows.h>

extern const unsigned tf3_vk_count;
extern void *tf3_vk_slots[];
extern const char *const tf3_vk_names[];

static HMODULE g_self;

/* A slot whose export is missing in the user's (older) loader: behave like a failing Vulkan call. */
static long long missing_export(void) { return -9; /* VK_ERROR_INCOMPATIBLE_DRIVER */ }

static void load_real_loader(void)
{
    wchar_t path[MAX_PATH];
    UINT n = GetSystemDirectoryW(path, MAX_PATH);
    if (n == 0 || n > MAX_PATH - 16)
        return;
    lstrcatW(path, L"\\vulkan-1.dll");
    HMODULE real = LoadLibraryW(path);
    for (unsigned i = 0; i < tf3_vk_count; ++i) {
        FARPROC p = real ? GetProcAddress(real, tf3_vk_names[i]) : NULL;
        tf3_vk_slots[i] = p ? (void *)p : (void *)missing_export;
    }
}

/* Directory of this DLL with a trailing backslash. */
static int self_dir(wchar_t *buf, DWORD cap)
{
    DWORD n = GetModuleFileNameW(g_self, buf, cap);
    if (n == 0 || n >= cap)
        return 0;
    while (n > 0 && buf[n - 1] != L'\\' && buf[n - 1] != L'/')
        --n;
    buf[n] = 0;
    return (int)n;
}

static int host_is_game(void)
{
    wchar_t exe[MAX_PATH];
    DWORD n = GetModuleFileNameW(NULL, exe, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        return 0;
    const wchar_t *base = exe + n;
    while (base > exe && base[-1] != L'\\' && base[-1] != L'/')
        --base;
    return lstrcmpiW(base, L"TransportFever3.exe") == 0;
}

static DWORD WINAPI load_plugin(LPVOID unused)
{
    (void)unused;
    wchar_t dir[MAX_PATH], path[MAX_PATH];
    if (!self_dir(dir, MAX_PATH - 40))
        return 0;
    lstrcpyW(path, dir);
    lstrcatW(path, L"tf3snap\\disabled");
    if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES)
        return 0;
    lstrcpyW(path, dir);
    lstrcatW(path, L"tf3snap\\tf3snap.dll");
    if (!LoadLibraryW(path)) {
        /* leave a breadcrumb; the plugin itself is what normally writes logs */
        wchar_t err[MAX_PATH];
        lstrcpyW(err, dir);
        lstrcatW(err, L"tf3snap\\proxy_error.txt");
        HANDLE h = CreateFileW(err, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            char msg[128];
            DWORD code = GetLastError(), w;
            wsprintfA(msg, "LoadLibrary(tf3snap\\tf3snap.dll) failed, GetLastError=%lu\r\n", code);
            WriteFile(h, msg, lstrlenA(msg), &w, NULL);
            CloseHandle(h);
        }
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = inst;
        DisableThreadLibraryCalls(inst);
        load_real_loader();
        if (host_is_game()) {
            /* not from DllMain itself: LoadLibrary under the loader lock is asking for trouble */
            HANDLE t = CreateThread(NULL, 0, load_plugin, NULL, 0, NULL);
            if (t)
                CloseHandle(t);
        }
    }
    return TRUE;
}
