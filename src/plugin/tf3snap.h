// Better Snap Option (0.2.x: "TF3 Snap Options") - shared declarations.
//
// Option B design: an in-process plugin (loaded by the vulkan-1.dll proxy) that
//  - installs MinHook detours on the game's snapping functions (hooks.cpp), and
//  - draws a small borderless top-most ImGui window of its own D3D11 device (overlay.cpp),
//    which shows while the road / track tool is open and the game is in front (nothing else).
// Nothing is written to the game's files; unload (remove the proxy or the 'disabled' marker) restores all.
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <atomic>
#include <cstdint>
#include <string>

#include "version.h"   // TF3SNAP_VERSION, shared with the DLL version resources

namespace tf3 {

// Panel languages (text.cpp has the wording, overlay.cpp the font for each).
enum { kLangZhTW, kLangZhCN, kLangEn, kLangJa, kLangCount };

// ---- options shared between the overlay thread (writer) and the game thread hooks (readers) --------
// Each field is atomic; the fields are independent so a half-applied change is at worst one odd frame.
// Two groups, as in the panel: 貼齊 = whether the cursor is pulled, 引導線 = which guides exist / are drawn.
struct Options {
    // 貼齊
    std::atomic<bool> existing{true};     // 1 貼齊既有幾何
    std::atomic<bool> length{false};      // 2 貼齊長度刻度
    std::atomic<bool> sides{true};        // 3 貼齊道路側邊（並排，不重疊：offset != 0, no flag）
    std::atomic<bool> lanes{true};        // 4 貼齊車道（與既有道路重疊：offset != 0, flag）
    std::atomic<bool> snapGuides{true};   // 5 貼齊引導線
    std::atomic<bool> nearby{true};       // 6 貼齊鄰近路口（節點間隙）
    std::atomic<float> lengthStep{8.0f};
    std::atomic<float> scale{1.0f};       // 貼齊範圍倍率
    // 引導線 (each kind switch applies to both showing and snapping)
    std::atomic<bool> showGuides{true};     // draw guides at all
    std::atomic<bool> guideAngle{true};     // +-45 / +-90 ... candidates (existing flag 0)
    std::atomic<int> grid{0};               // 角度格線（度），0 = 遊戲預設
    std::atomic<bool> guideStraight{true};  // straight-on along existing roads (existing flag 1), original
    std::atomic<bool> guideCross{true};     // ours: perpendicular through nearby road ends
    // panel
    std::atomic<bool> toolOnly{true};   // only while the road / track tool is open
    std::atomic<int> lang{kLangZhTW};   // panel language (kLang*)
    std::atomic<int> panelX{40}, panelY{120};
    // status (not saved)
    std::atomic<bool> hooksOk{false};     // signatures matched and detours installed
    std::atomic<bool> crossOk{false};     // everything the cross guide needs was found
    std::atomic<bool> toolDetect{false};  // StreetBuilder::Step is hooked: we can tell when the tool is open
};
extern Options g_opt;
// What "恢復遊戲預設" restores: the game's own behaviour (the cross guide is ours, so it goes off).
void ResetToStock();

// Module base of TransportFever3.exe and helpers.
uintptr_t GameBase();

// ---- logging (log.cpp) ----------------------------------------------------------------------------
void LogInit();
void Logf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));   // arguments checked like printf

// ---- signature scan (sigscan.cpp) -----------------------------------------------------------------
// Find a masked byte pattern ("48 8B ?? ..") in the game's .text; returns 0 if not found or not unique.
uintptr_t FindUniquePattern(const char* pattern);
// Convenience: all target addresses resolved once at startup.
struct Targets {
    uintptr_t calcCursorPoint, findSnapBaseEdge, myFindSnapPointNew, roadEarlyAbort,
        trackNearScore, trackFarScore, collectNodesAndEdges, getSnapTangents,
        operatorNew, operatorDelete;
    uintptr_t coneCallSite;   // return address inside findSnapBaseEdge that calls collectNodesAndEdges
    // optional (cross-alignment guide + telling the guide-drawing call apart)
    uintptr_t updateRenderer, drawEdges, makeStraightEdge, getComponentTypeIndex, componentData,
        heightAt, snapPointPos;
    uintptr_t tangentsDrawCallSite;   // return address inside updateRenderer that calls getSnapTangents
    uintptr_t baseEdgeTypeInfo;       // ecs::component::BaseEdge type_info (found by its RTTI name)
    // optional (panel shows only while the road / track tool is open)
    uintptr_t streetBuilderStep;
};
extern Targets g_t;
bool ResolveTargets();       // fills g_t via signatures; false if any required one is missing
                             // (the optional ones only switch off g_opt.crossOk)

// ---- game state (gamestate.cpp) -------------------------------------------------------------------
bool InGame();           // a save is loaded (g_gameUI != null)
bool Readable(uintptr_t addr, size_t len);   // committed, readable, not a guard page (VirtualQuery)

// ---- hooks (hooks.cpp) ----------------------------------------------------------------------------
bool InstallHooks();      // MinHook init + all detours; needs ResolveTargets() first
void RemoveHooks();
bool BuilderToolOpen(uint64_t graceMs);   // road / track tool open: StreetBuilder::Step ran within graceMs

// ---- config (config.cpp) --------------------------------------------------------------------------
std::wstring PluginDir();            // ...\tf3snap\ (ends with a backslash)
std::wstring ConfigPath();           // ...\tf3snap\tf3snap.ini
void LoadConfig();
void SaveConfig();

// ---- panel wording (text.cpp): tf3snap\tf3snap_text_<code>.ini per language, reloaded when saved -----
const char* LangCode(int lang);      // "zh-TW", "zh-CN", "en", "ja" (file suffix, tf3snap.ini value)
const char* LangName(int lang);      // the language's own name, for the language list
int LangFromCode(const std::string& code);   // -1 if unknown
int LangFromSystem();                // Windows display language, English if none of ours
void TextInit();                     // writes missing templates, then loads g_opt.lang's file
bool TextPoll();                     // reload if g_opt.lang or its file changed; true when it did
const char* T(const char* key);      // text for key (file override, else built-in text of the language)

// ---- overlay (overlay.cpp) ------------------------------------------------------------------------
void OverlayRun();        // blocking; runs the ImGui window loop on its own thread until the game exits
void OverlayStop();

}  // namespace tf3
