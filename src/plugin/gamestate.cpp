// Read the game's own state so the overlay can hide itself when it should.
// Both facts were verified live (see re/NOTES.md section 12):
//   g_gameUI  (exe + 0x403C008): null on main menu / loading, non-null once a save is in play.
//   *(g_gameUI + 0xB90) (u8)   : 1 while the ESC in-game ("遊戲暫停中") menu is open, else 0.
#include "tf3snap.h"

namespace tf3 {

// RVAs are from the reference build.  They are plain data globals, not code, so they are not
// signature-scannable; if a game update moves them the worst case is the overlay not auto-hiding,
// never a crash (every dereference is guarded by Readable()).
static constexpr uintptr_t RVA_G_GAMEUI = 0x403C008;
static constexpr uintptr_t OFF_MENU_PAUSE = 0xB90;

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

static uintptr_t GameUIPtr() {
    uintptr_t base = GameBase();
    if (!base) return 0;
    uintptr_t slot = base + RVA_G_GAMEUI;
    if (!Readable(slot, sizeof(uintptr_t))) return 0;
    uintptr_t ui = *reinterpret_cast<uintptr_t*>(slot);
    return Readable(ui, OFF_MENU_PAUSE + 1) ? ui : 0;
}

bool InGame() { return GameUIPtr() != 0; }

bool MenuPauseOpen() {
    uintptr_t ui = GameUIPtr();
    if (!ui) return false;
    return *reinterpret_cast<uint8_t*>(ui + OFF_MENU_PAUSE) != 0;
}

}  // namespace tf3
