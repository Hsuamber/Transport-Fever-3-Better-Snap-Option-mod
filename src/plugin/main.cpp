// tf3snap.dll entry point.  Loaded by the vulkan-1.dll proxy only inside TransportFever3.exe.
// An init thread wires everything up; the overlay loop runs there until the game exits.
#include "tf3snap.h"

namespace tf3 {

static HANDLE g_thread = nullptr;

static DWORD WINAPI InitThread(LPVOID) {
    LogInit();
    Logf(TF3SNAP_PRODUCT " v%s starting; game base %p", TF3SNAP_VERSION, (void*)GameBase());
    LoadConfig();

    if (ResolveTargets() && InstallHooks()) {
        g_opt.hooksOk = true;
        Logf("hooks active");
    } else {
        g_opt.hooksOk = false;
        Logf("hooks NOT active (game left untouched); panel will still open to show status");
    }

    OverlayRun();   // blocks until OverlayStop() (DLL unload) or the window loop ends
    return 0;
}

}  // namespace tf3

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved) {
    using namespace tf3;
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(inst);
            g_thread = CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
            break;
        case DLL_PROCESS_DETACH:
            // reserved != null => the process is terminating; let the OS reclaim everything (touching
            // MinHook/D3D during teardown is unsafe).  reserved == null => a real FreeLibrary: clean up.
            if (reserved == nullptr) {
                OverlayStop();
                if (g_thread) {
                    WaitForSingleObject(g_thread, 2000);
                    CloseHandle(g_thread);
                    g_thread = nullptr;
                }
                RemoveHooks();
                SaveConfig();
            }
            break;
    }
    return TRUE;
}
