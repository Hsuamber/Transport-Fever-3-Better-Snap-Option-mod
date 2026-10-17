// Read the game's own state: whether a save is in play (used only when the tool check is off or unavailable).
//   g_gameUI (exe + 0x403C008): null on main menu / loading, non-null once a save is in play (re/NOTES.md 12).
// Nothing else is read.  0.3.1 also hid the panel on *(g_gameUI + 0xB90), taken for "ESC menu open", but the
// game sets it on any pause (the speed bar's pause button too), so the panel vanished whenever paused.
#include "tf3snap.h"

namespace tf3 {

// RVA from the reference build.  A plain data global, not code, so not signature-scannable; if a game update
// moves it the worst case is the panel not hiding on the main menu, never a crash (guarded by Readable()).
static constexpr uintptr_t RVA_G_GAMEUI = 0x403C008;

bool Readable(uintptr_t addr, size_t len) {
    if (!addr) return false;
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery(reinterpret_cast<void*>(addr), &mbi, sizeof(mbi))) return false;
    if (mbi.State != MEM_COMMIT) return false;
    DWORD ok = PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_WRITECOPY;
    if (!(mbi.Protect & ok) || (mbi.Protect & PAGE_GUARD)) return false;
    uintptr_t end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    return addr + len <= end;
}

bool InGame() {
    uintptr_t base = GameBase();
    if (!base) return false;
    uintptr_t slot = base + RVA_G_GAMEUI;
    if (!Readable(slot, sizeof(uintptr_t))) return false;
    return Readable(*reinterpret_cast<uintptr_t*>(slot), sizeof(uintptr_t));
}

}  // namespace tf3
