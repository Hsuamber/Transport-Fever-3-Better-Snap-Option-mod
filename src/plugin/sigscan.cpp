// Signature scanning: resolve the game functions by masked byte pattern so the plugin keeps working
// across game updates (addresses move; the function prologues usually do not).
#include "tf3snap.h"

#include <emmintrin.h>   // SSE2 (every x64 CPU)

#include <cstring>
#include <vector>

namespace tf3 {

Options g_opt;
Targets g_t;

uintptr_t GameBase() { return reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)); }

static bool TextRange(const uint8_t*& begin, size_t& size) {
    uintptr_t base = GameBase();
    if (!base) return false;
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return false;
    auto sec = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (memcmp(sec[i].Name, ".text", 5) == 0) {
            begin = reinterpret_cast<const uint8_t*>(base + sec[i].VirtualAddress);
            size = sec[i].Misc.VirtualSize;
            return true;
        }
    }
    return false;
}

// Parse "48 8B ?? 10" into bytes + mask (mask[i]=false means wildcard).
static bool ParsePattern(const char* p, std::vector<uint8_t>& bytes, std::vector<bool>& mask) {
    for (; *p; ) {
        if (*p == ' ') { ++p; continue; }
        if (p[0] == '?' ) {
            bytes.push_back(0);
            mask.push_back(false);
            p += (p[1] == '?') ? 2 : 1;
        } else {
            auto hx = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            int hi = hx(p[0]), lo = hx(p[1]);
            if (hi < 0 || lo < 0) return false;
            bytes.push_back(static_cast<uint8_t>((hi << 4) | lo));
            mask.push_back(true);
            p += 2;
        }
    }
    return !bytes.empty();
}

// Each pattern is anchored on its rarest fixed byte (by a sampled histogram of .text, built once - only the
// init thread scans), and positions holding that byte are found 16 at a time with SSE2.  0.2.0 tried every
// position of the 54 MB .text with a vector<bool> mask: ~150-250 ms per pattern, seconds for all of them.
static const uint32_t* TextHistogram(const uint8_t* begin, size_t size) {
    static uint32_t hist[256];
    static bool done = false;
    if (!done) {
        for (size_t i = 0; i < size; i += 61) ++hist[begin[i]];   // a sample is plenty to rank bytes
        done = true;
    }
    return hist;
}

uintptr_t FindUniquePattern(const char* pattern) {
    const uint8_t* begin = nullptr;
    size_t size = 0;
    if (!TextRange(begin, size)) return 0;
    std::vector<uint8_t> b;
    std::vector<bool> m;
    if (!ParsePattern(pattern, b, m)) return 0;
    const size_t n = b.size();
    if (n > size) return 0;
    const uint32_t* hist = TextHistogram(begin, size);
    size_t anchor = n;
    for (size_t k = 0; k < n; ++k)
        if (m[k] && (anchor == n || hist[b[k]] < hist[b[anchor]])) anchor = k;
    if (anchor == n) return 0;   // all wildcards
    const std::vector<uint8_t> fixed(m.begin(), m.end());   // plain bytes for the inner loop
    uintptr_t hit = 0;
    int count = 0;
    auto check = [&](size_t s) -> bool {   // false = second match: not unique
        for (size_t k = 0; k < n; ++k)
            if (fixed[k] && begin[s + k] != b[k]) return true;
        hit = reinterpret_cast<uintptr_t>(begin + s);
        return ++count < 2;
    };
    const size_t lastStart = size - n;
    const __m128i needle = _mm_set1_epi8(static_cast<char>(b[anchor]));
    size_t s0 = 0;
    for (; s0 + anchor + 16 <= size; s0 += 16) {   // starts s0..s0+15, never reading past .text
        unsigned bits = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(
            _mm_loadu_si128(reinterpret_cast<const __m128i*>(begin + s0 + anchor)), needle));
        while (bits) {
            const size_t s = s0 + (size_t)__builtin_ctz(bits);
            bits &= bits - 1;
            if (s > lastStart) break;
            if (!check(s)) return 0;   // not unique -> refuse (safer than hooking the wrong place)
        }
    }
    for (size_t s = s0; s <= lastStart; ++s)   // the last few starts
        if (begin[s + anchor] == b[anchor] && !check(s)) return 0;
    return count == 1 ? hit : 0;
}

// `func + retOff` must be the return address of a `call rel32` (E8) into `callee`; 0 otherwise.
static uintptr_t CallSiteTo(uintptr_t func, uintptr_t retOff, uintptr_t callee, const char* what) {
    if (!func || !callee) return 0;
    const uintptr_t ret = func + retOff;
    const uint8_t* ins = reinterpret_cast<const uint8_t*>(ret - 5);
    int32_t rel;
    memcpy(&rel, ins + 1, 4);
    if (ins[0] == 0xE8 && ret + (intptr_t)rel == callee) return ret;
    Logf("%s: no call at +0x%llx (game changed?)", what, (unsigned long long)retOff);
    return 0;
}

// An MSVC type_info is {vftable, spare, decorated name...}: find the unique RTTI name in .data and step
// back 0x10.  Data addresses move with every game update; the name does not.
static uintptr_t FindTypeInfo(const char* decorated) {
    uintptr_t base = GameBase();
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    auto sec = IMAGE_FIRST_SECTION(nt);
    const size_t len = strlen(decorated) + 1;   // include the terminator so a longer name cannot match
    for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (memcmp(sec[i].Name, ".data", 6) != 0) continue;
        const char* b = reinterpret_cast<const char*>(base + sec[i].VirtualAddress);
        const size_t n = sec[i].SizeOfRawData < sec[i].Misc.VirtualSize ? sec[i].SizeOfRawData : sec[i].Misc.VirtualSize;
        uintptr_t hit = 0;
        for (size_t k = 0x10; k + len <= n; ++k) {
            const void* c = memchr(b + k, decorated[0], n - len + 1 - k);
            if (!c) break;
            k = static_cast<size_t>(static_cast<const char*>(c) - b);
            if (memcmp(b + k, decorated, len) == 0) {
                if (hit) return 0;   // not unique
                hit = reinterpret_cast<uintptr_t>(b + k - 0x10);
            }
        }
        return hit;
    }
    return 0;
}

bool ResolveTargets() {
    struct Entry { const char* id; uintptr_t* out; uint32_t refRva; const char* pat; };
    g_t = {};
#define SIG(id, rva, pat) Entry{#id, nullptr, (uint32_t)(rva), pat},
    // Map the generated SIG rows to the fields we need.
    struct Row { const char* id; uint32_t refRva; const char* pat; };
    static const Row rows[] = {
#undef SIG
#define SIG(id, rva, pat) Row{#id, (uint32_t)(rva), pat},
#include "sigs.inc"
#undef SIG
    };

    auto get = [&](const char* id) -> uintptr_t {
        for (const auto& r : rows)
            if (strcmp(r.id, id) == 0) {
                uintptr_t a = FindUniquePattern(r.pat);
                Logf("sig %-22s %s @ %p", id, a ? "OK " : "MISS", (void*)a);
                return a;
            }
        Logf("sig %-22s not in table", id);
        return 0;
    };

    g_t.calcCursorPoint = get("CalcCursorPoint");
    g_t.findSnapBaseEdge = get("FindSnapPointBaseEdge");
    g_t.myFindSnapPointNew = get("MyFindSnapPointNew");
    g_t.roadEarlyAbort = get("RoadEarlyAbort");
    g_t.trackNearScore = get("TrackNearScore");
    g_t.trackFarScore = get("TrackFarScore");
    g_t.collectNodesAndEdges = get("CollectNodesAndEdges");
    g_t.getSnapTangents = get("GetSnapTangents");
    g_t.operatorNew = get("OperatorNew");
    g_t.operatorDelete = get("OperatorDelete");

    // The cone query must only be scaled for the FindSnapPointBaseEdge caller.  In the reference build
    // that call returns to findSnapBaseEdge+0x221 (0x60c581).  Derive it from the resolved base so it
    // tracks the real function, and verify there really is a call to it there.
    g_t.coneCallSite = CallSiteTo(g_t.findSnapBaseEdge, 0x221, g_t.collectNodesAndEdges, "cone call site");

    bool ok = g_t.calcCursorPoint && g_t.findSnapBaseEdge && g_t.myFindSnapPointNew &&
              g_t.roadEarlyAbort && g_t.trackNearScore && g_t.trackFarScore &&
              g_t.collectNodesAndEdges && g_t.getSnapTangents && g_t.operatorNew && g_t.operatorDelete;
    Logf("ResolveTargets: %s", ok ? "all found" : "MISSING some (hooks disabled, game untouched)");

    // Optional: the cross-alignment guide, and telling the guide-drawing call apart from the snapping ones.
    g_t.updateRenderer = get("UpdateRenderer");
    g_t.drawEdges = get("DrawEdges");
    g_t.makeStraightEdge = get("MakeStraightEdge");
    g_t.getComponentTypeIndex = get("GetComponentTypeIndex");
    g_t.componentData = get("ComponentData");
    g_t.heightAt = get("HeightAt");
    g_t.snapPointPos = get("SnapPointPos");
    // UpdateRenderer calls GetSnapTangents at +0x3ad (returns to 0x58a4b2 in the reference build)
    g_t.tangentsDrawCallSite = CallSiteTo(g_t.updateRenderer, 0x3b2, g_t.getSnapTangents, "guide draw call site");
    g_t.baseEdgeTypeInfo = FindTypeInfo(".?AUBaseEdge@component@ecs@@");
    Logf("BaseEdge type_info @ %p", (void*)g_t.baseEdgeTypeInfo);
    bool cross = ok && g_t.updateRenderer && g_t.drawEdges && g_t.makeStraightEdge &&
                 g_t.getComponentTypeIndex && g_t.componentData && g_t.heightAt && g_t.snapPointPos &&
                 g_t.baseEdgeTypeInfo;
    g_opt.crossOk = cross;
    Logf("cross guide targets: %s", cross ? "all found" : "MISSING some (cross guide off, rest unaffected)");
    if (!g_t.tangentsDrawCallSite) Logf("guide draw call site not found: 顯示引導線 has no effect");

    // Optional: the panel follows the road / track tool (missing -> the panel just ignores that setting).
    g_t.streetBuilderStep = get("StreetBuilderStep");
    return ok;
}

}  // namespace tf3
