// MinHook detours on the game's snapping functions - a direct C++ port of the Frida agent in
// re/tools/snaptool.py (that logic is the tested reference).  Offsets and behaviour are documented
// in re/NOTES.md (sections 2-4, 11, 13).  Every edit is gated so the game is only affected while the
// road/track builder's cursor is active (depth>0), matching the Frida version.
//
// ABI notes (why the prototypes look the way they do):
//   Win64 passes args 1-4 in RCX/RDX/R8/R9, with a FLOAT arg in that position going to XMM0-3 instead.
//   So a register-position float MUST be typed float (RoadEarlyAbort's 2nd arg); a float in a stack
//   position (arg 5+) can be typed float too but an all-void* slot would also forward it intact.
//   Arities are taken from the Ghidra decompiles (re/decomp/*.c).
#include "tf3snap.h"

#include "MinHook.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <mutex>
#include <vector>

namespace tf3 {

// per-UI-thread state (the builder cursor path is single-threaded; thread_local keeps it clean)
static thread_local int t_depth = 0;       // >0 while inside CalcCursorPoint
static thread_local bool t_fresh = false;  // true until the shared scoring ctx is tuned once per find
static thread_local bool t_isRoad = true;  // current FindSnapPointBaseEdge scores as a road (not a track)

// One CalcCursorPoint call, as far as the cross guide needs it.  All pointers are owned by the caller
// (StreetBuilder::Step) or are long-lived, so they are still valid after CalcCursorPoint returns.
struct CursorCall {
    void* out;        // SnapPoint (0x34): edge@0 (-1 free), t@4, offset@8, flags@0xc (byte 0xd: terrain),
                      //                   pos@0x10, w@0x1c, cached pos@0x20 valid when byte 0x2c
    void* view;       // engine::View: +4 viewport h (int), +0x10 vertical fov, +0x24 camera position
    void* cursor;     // CursorPos: +8 world, +0x14 hasWorld
    void* toolkit;    // StreetToolkit: +0xa8 terrain, +0xb0 engine, +0xc0 cone system
    void* start;      // SnapPointInfo* (pos@0) while dragging the second point, else null
    void* node2seg;   // std::map<Entity, std::set<Entity>>, filled from FindSnapPointBaseEdge's 9th arg
};
static thread_local CursorCall* t_call = nullptr;

static void* (*oOpNew)(uint64_t) = nullptr;
static void (*oOpDel)(void*, uint64_t) = nullptr;

// Buffers of the game's std::vectors must be allocated and freed the way MSVC's std::allocator does it: a block
// of 4096 bytes or more is over-allocated by 0x27, the user pointer aligned to 32 and the real block start kept
// just before it (FindSnapPointBaseEdge frees the cone-query vector exactly so).  Freeing such a buffer with a
// plain operator delete hands the heap an interior pointer: heap corruption, 0xC0000374 (0.2.x/0.3.0 crash on
// a dense map, once the cone query returned 1024+ entities).
static constexpr uint64_t kBigAlloc = 0x1000, kBigExtra = 0x27;

static void* GameVecAlloc(uint64_t bytes) {
    if (bytes < kBigAlloc) return oOpNew(bytes);
    const uintptr_t block = reinterpret_cast<uintptr_t>(oOpNew(bytes + kBigExtra));
    if (!block) return nullptr;
    void* p = reinterpret_cast<void*>((block + kBigExtra) & ~uintptr_t(31));
    static_cast<uintptr_t*>(p)[-1] = block;
    return p;
}

static void GameVecFree(void* p, uint64_t bytes) {
    if (!p) return;
    if (bytes >= kBigAlloc) {
        const uintptr_t block = static_cast<const uintptr_t*>(p)[-1];
        const uintptr_t shift = reinterpret_cast<uintptr_t>(p) - block;
        if (shift < 8 || shift > kBigExtra) {   // not laid out as expected: leak it rather than corrupt the heap
            static bool s_logged = false;
            if (!s_logged) Logf("GameVecFree: unexpected big-block header at %p (shift %llu), leaking it", p,
                                (unsigned long long)shift);
            s_logged = true;
            return;
        }
        p = reinterpret_cast<void*>(block);
        bytes += kBigExtra;
    }
    oOpDel(p, bytes);
}

static inline float fget(const void* p, size_t off) { return *reinterpret_cast<const float*>((const char*)p + off); }
static inline void fset(void* p, size_t off, float v) { *reinterpret_cast<float*>((char*)p + off) = v; }
static inline uint8_t bget(const void* p, size_t off) { return *reinterpret_cast<const uint8_t*>((const char*)p + off); }
static inline void bset(void* p, size_t off, uint8_t v) { *reinterpret_cast<uint8_t*>((char*)p + off) = v; }
static inline int32_t iget(const void* p, size_t off) { return *reinterpret_cast<const int32_t*>((const char*)p + off); }
static inline void* pget(const void* p, size_t off) { return *reinterpret_cast<void* const*>((const char*)p + off); }
static inline bool on(const std::atomic<bool>& b) { return b.load(std::memory_order_relaxed); }

static void ApplyCross(const CursorCall& x);

// ---- 571820 CalcCursorPoint: depth counter + the cross guide on its result (18 integer-class args) ---
// Returns its result through a hidden out pointer, and like every such function hands that pointer back in
// RAX - which the click handler 580900 reads right after the call (vmovups ymm0,[rax] at 580f95).  A void
// detour leaves RAX as garbage there: that was the 0.2.0 crash on clicking with the road tool.  So every
// detour below returns exactly what the original returned.
typedef void* (*CalcFn)(void*, void*, void*, void*, void*, void*, void*, void*, void*,
                        void*, void*, void*, void*, void*, void*, void*, void*, void*);
static CalcFn oCalc = nullptr;
static void* hCalc(void* a0, void* a1, void* a2, void* a3, void* a4, void* a5, void* a6, void* a7,
                  void* a8, void* a9, void* a10, void* a11, void* a12, void* a13, void* a14,
                  void* a15, void* a16, void* a17) {
    // CalcCursorPoint(out, view, cursor, toolkit, bool, fine, reference, start, ...)
    CursorCall call{a0, a1, a2, a3, a7, nullptr};
    const bool outer = t_depth == 0;
    if (outer) t_call = &call;
    ++t_depth;
    void* ret = oCalc(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17);
    --t_depth;
    if (outer) {
        t_call = nullptr;
        ApplyCross(call);
    }
    return ret;
}

// ---- 60c360 FindSnapPointBaseEdge: option 1 (existing) and 2 (length) on the result -------------
// (out, engine, terrain, octree, collision, streetConn, railroadCrossing, parallelStrip, node2seg,
//  cursor, view, reference, start, ignore, filter, cfg) - 16 integer-class args; returns out in RAX.
typedef void* (*FindFn)(void*, void*, void*, void*, void*, void*, void*, void*, void*,
                        void*, void*, void*, void*, void*, void*, void*);
static FindFn oFind = nullptr;
static void* hFind(void* out, void* a1, void* a2, void* a3, void* a4, void* a5, void* a6, void* a7,
                   void* node2seg, void* cur, void* view, void* ref, void* start, void* a13, void* a14,
                   void* cfg) {
    const bool mine = t_depth > 0;
    if (mine) {
        t_fresh = true;
        if (t_call) t_call->node2seg = node2seg;
        // SnapConfig +10 forceRoad; SegmentAndEntity +0x120 type (0 = street, 1 = track)
        t_isRoad = (cfg && bget(cfg, 10) != 0) || (ref && iget(ref, 0x120) == 0);
    }
    void* ret = oFind(out, a1, a2, a3, a4, a5, a6, a7, node2seg, cur, view, ref, start, a13, a14, cfg);
    if (mine) {
        t_fresh = false;
        // result: optional<variant<SnapPointTerrain, SnapPointBaseEdge>>  +0x10 index (0 terrain, 1 edge), +0x14 has_value
        if (!on(g_opt.existing)) {
            // replace the snapped edge point with the raw terrain point under the cursor
            fset(out, 0x0, fget(cur, 0x8)); fset(out, 0x4, fget(cur, 0xc)); fset(out, 0x8, fget(cur, 0x10));
            fset(out, 0xc, 0.0f);
            bset(out, 0x10, 0);
            bset(out, 0x14, bget(cur, 0x14) ? 1 : 0);
        }
        float step = g_opt.lengthStep.load(std::memory_order_relaxed);
        if (on(g_opt.length) && step > 0.0f && start && bget(out, 0x14) != 0 && bget(out, 0x10) == 0) {
            float sx = fget(start, 0x0), sy = fget(start, 0x4);   // SnapPointInfo.pos
            float dx = fget(out, 0x0) - sx, dy = fget(out, 0x4) - sy;
            float L = std::sqrt(dx * dx + dy * dy);
            if (L > 0.5f) {
                float mul = (std::max)(1.0f, std::round(L / step)) * step / L;
                fset(out, 0x0, sx + dx * mul);
                fset(out, 0x4, sy + dy * mul);
            }
        }
    }
    return ret;
}

// ---- 5fb370 MyFindSnapPointNew: the "fine" flag (arg 13) generates no parallel / lane offsets ------
// Used when both offset kinds are off, and for tracks when side-by-side is off (tracks have no lane
// offsets and their scorers return floats, see re/NOTES.md 13.1).  The snap radius is not shrunk.
// 17 integer-class args (arg9 float is a stack slot, forwarded intact as void*).
typedef void* (*MyFindFn)(void*, void*, void*, void*, void*, void*, void*, void*, void*,
                          void*, void*, void*, void*, void*, void*, void*, void*);
static MyFindFn oMyFind = nullptr;
static void* hMyFind(void* a0, void* a1, void* a2, void* a3, void* a4, void* a5, void* a6, void* a7,
                    void* a8, void* a9, void* a10, void* a11, void* fine, void* a13, void* a14,
                    void* a15, void* a16) {
    if (t_depth > 0 && !on(g_opt.sides) && (!on(g_opt.lanes) || !t_isRoad))
        fine = reinterpret_cast<void*>(1);
    return oMyFind(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, fine, a13, a14, a15, a16);
}

// ---- scoring-lambda captures: option 6 (nearby = nodeGap) and the range multiplier --------------
// Shared capture object is reused for every candidate in one find, so tune it only once (t_fresh).
static void tune(void* ctx, size_t radOff, size_t gapOff) {
    if (t_depth <= 0 || !t_fresh) return;
    t_fresh = false;
    float sc = g_opt.scale.load(std::memory_order_relaxed);
    if (sc != 1.0f) fset(ctx, radOff, fget(ctx, radOff) * sc);
    if (!on(g_opt.nearby)) fset(ctx, gapOff, 0.0f);
}

// 60d090 RoadEarlyAbort: u64(longlong* ctx, float dist, u64, float*, SnapPointBaseEdge* sp, longlong)
// -> 1 = reject.  sp {edge, t, offset@8, flag@0xc}; MyFindSnapPointNew sets the flag on offsets that
// overlap the existing road (lane level, option 4); unflagged non-zero offsets are side-by-side (option 3).
typedef uint64_t (*RoadAbortFn)(void*, float, void*, void*, void*, void*);
static RoadAbortFn oRoadAbort = nullptr;
static uint64_t hRoadAbort(void* ctx, float dist, void* p3, void* p4, void* sp, void* p6) {
    tune(ctx, 0x8, 0xc);
    uint64_t r = oRoadAbort(ctx, dist, p3, p4, sp, p6);
    if (t_depth > 0 && sp && (r & 0xff) == 0 && fget(sp, 8) != 0.0f) {
        const bool lane = bget(sp, 0xc) != 0;
        if (lane ? !on(g_opt.lanes) : !on(g_opt.sides)) return 1;
    }
    return r;
}

// 605a10 TrackNearScore / 606b50 TrackFarScore: (captures, ..) returning the score (float, XMM0)
typedef float (*ScoreFn)(void*, void*, void*, void*);
static ScoreFn oTrackNear = nullptr, oTrackFar = nullptr;
static float hTrackNear(void* ctx, void* a1, void* a2, void* a3) { tune(ctx, 0x40, 0x44); return oTrackNear(ctx, a1, a2, a3); }
static float hTrackFar(void* ctx, void* a1, void* a2, void* a3) { tune(ctx, 0x40, 0x44); return oTrackFar(ctx, a1, a2, a3); }

// 60b370 CollectNodesAndEdges: void(out vector<Entity>*, engine, coneSys, view, cursor, float R px /*arg6,
// stack*/, float maxDist) - scale the pick cone for range.  The cross guide calls the original directly.
typedef void* (*CollectFn)(void*, void*, void*, void*, void*, float, float);
static CollectFn oCollect = nullptr;
static void* hCollect(void* p1, void* p2, void* p3, void* p4, void* p5, float R, float maxDist) {
    float sc = g_opt.scale.load(std::memory_order_relaxed);
    if (t_depth > 0 && sc != 1.0f &&
        reinterpret_cast<uintptr_t>(__builtin_return_address(0)) == g_t.coneCallSite) {
        R *= sc;
    }
    return oCollect(p1, p2, p3, p4, p5, R, maxDist);
}

// ---- 57dcc0 GetSnapTangents: which guides exist, separately for drawing and for snapping ----------
// vector<TangentCand> at *out: [begin@0, end@8, cap@16]; TangentCand = 0x14 bytes {dx,dy,existing@8,dz@0xc,has3D@0x10}
//   existing = 1: directions of existing roads / the reference tangent -> 縱向 (straight-on) guides
//   existing = 0: the +-45 / +-90 ... offsets from them                -> 角度 guides (grid applies here)
// Callers: UpdateRenderer (draws the white dashed lines) vs 576ed0 / MakeEdgesAndSections (pull the
// point / tangent onto them), told apart by the return address.
typedef void* (*TangentsFn)(void*, void*, void*, void*, uint8_t, void*);
static TangentsFn oTangents = nullptr;

static void filterCandidates(void* out, bool keepStraight, bool keepAngle, int stepDeg) {
    char** v = reinterpret_cast<char**>(out);
    char* b = v[0];
    char* e = v[1];
    if (!b || b == e) return;
    if (keepStraight && keepAngle && stepDeg <= 0) return;
    const int n = (int)((e - b) / 0x14);
    if (n <= 0 || n > 64) return;
    char straight[64][0x14], angle[64][0x14];
    double straightAng[64];
    int ns = 0, na = 0;
    for (int i = 0; i < n; ++i) {
        char* p = b + i * 0x14;
        if (bget(p, 8)) {
            memcpy(straight[ns], p, 0x14);
            straightAng[ns++] = std::atan2(fget(p, 4), fget(p, 0));
        } else {
            memcpy(angle[na++], p, 0x14);
        }
    }
    double dirs[72];
    int nd = 0;
    bool useGrid = keepAngle && stepDeg > 0 && ns > 0;
    if (useGrid) {
        for (int k = 1; k * stepDeg < 360.0 - 1e-3 && nd < 72; ++k) {
            double a = straightAng[0] + k * stepDeg * 3.14159265358979323846 / 180.0;
            bool dup = false;
            for (int j = 0; j < ns; ++j)
                if (std::fabs(std::atan2(std::sin(a - straightAng[j]), std::cos(a - straightAng[j]))) < 0.01) { dup = true; break; }
            if (!dup) dirs[nd++] = a;
        }
    }
    const int keepS = keepStraight ? ns : 0;
    const int keepA = (keepAngle && !useGrid) ? na : 0;
    const int total = keepS + keepA + nd;
    char* nb = b;   // shrinking: rewrite in place (everything was copied out above)
    if (total > n) {
        nb = reinterpret_cast<char*>(GameVecAlloc((uint64_t)total * 0x14));
        if (!nb) return;
    }
    char* p = nb;
    for (int i = 0; i < keepS; ++i) { memcpy(p, straight[i], 0x14); p += 0x14; }
    for (int i = 0; i < keepA; ++i) { memcpy(p, angle[i], 0x14); p += 0x14; }
    for (int i = 0; i < nd; ++i) {
        fset(p, 0, (float)std::cos(dirs[i]));
        fset(p, 4, (float)std::sin(dirs[i]));
        *reinterpret_cast<uint32_t*>(p + 8) = 0;
        fset(p, 0xc, 0.0f);
        *reinterpret_cast<uint32_t*>(p + 0x10) = 0;
        p += 0x14;
    }
    if (nb == b) { v[1] = p; return; }
    char* cap = v[2];
    GameVecFree(b, (uint64_t)(cap - b));
    v[0] = nb; v[1] = p; v[2] = p;
}

static void* hTangents(void* out, void* a1, void* a2, void* a3, uint8_t a4, void* a5) {
    const bool draw = g_t.tangentsDrawCallSite &&
        reinterpret_cast<uintptr_t>(__builtin_return_address(0)) == g_t.tangentsDrawCallSite;
    void* ret = oTangents(out, a1, a2, a3, a4, a5);
    const bool enabled = draw ? on(g_opt.showGuides) : on(g_opt.snapGuides);
    filterCandidates(out, enabled && on(g_opt.guideStraight), enabled && on(g_opt.guideAngle),
                     g_opt.grid.load(std::memory_order_relaxed));
    return ret;   // the vector header lives at `out`; filtering only rewrites its pointers
}

// =====================================================================================================
// Cross-alignment guide (橫向導引線).  Lines perpendicular to existing roads at their end nodes; a cursor
// point close to one is moved onto it so a new road ends flush with its neighbours, and a dashed line is
// drawn with the renderer the game uses for its own guides.  See re/NOTES.md 13.2.
// =====================================================================================================
typedef int (*TypeIndexFn)(void*, void*);                                   // (engine+0x48, type_info**)
typedef void* (*CompFn)(void*, int, int);                                    // (engine, entity, typeIndex)
typedef float (*HeightFn)(void*, const float*);                              // (terrain, xy*)
typedef void* (*SnapPosFn)(float*, void*, const void*, uint8_t);             // (out xyz, engine, SnapPoint*, useCache)
typedef void* (*StraightFn)(void*, const float*, const float*);             // (out 0x40, p0*, p1*)
typedef void (*DrawFn)(void*, void*, float, const float*, const float*);     // (renderer, vec*, width, colA*, colB*)
typedef void* (*RendererFn)(void*, void*);                                   // UpdateRenderer(builder, ..)
static TypeIndexFn fTypeIndex = nullptr;
static CompFn fComp = nullptr;
static HeightFn fHeight = nullptr;
static SnapPosFn fSnapPos = nullptr;
static StraightFn fStraight = nullptr;
static DrawFn fDraw = nullptr;
static RendererFn oRenderer = nullptr;

static constexpr float kTolPx = 14.0f, kConePx = 450.0f, kMaxDeg = 15.0f;
static constexpr uint64_t kRefreshMs = 150, kSnapTtlMs = 400;
static constexpr int kMaxEntities = 800, kMaxDash = 64;

struct Line { float px, py, pz, ux, uy; };   // end node position + road direction there (unit, xy)
struct Snap { bool valid; float x, y, z; Line from; uint64_t at; };

// game-thread only
static std::vector<Line> s_lines;
static uint64_t s_linesAt = 0;
static void* s_terrain = nullptr;
static void* s_idxEngine = nullptr;
static int s_edgeIdx = -1;
static bool s_crossBroken = false;   // a sanity check failed once: stay off for this session
// shared with UpdateRenderer (may be another thread)
static std::mutex s_snapMx;
static Snap s_snap{};

static void SetSnap(const Snap& s) {
    std::lock_guard<std::mutex> lk(s_snapMx);
    s_snap = s;
}

static void CrossBroken(const char* why) {
    if (!s_crossBroken) Logf("cross guide disabled for this session: %s", why);
    s_crossBroken = true;
}

static float TerrainZ(float x, float y, float fallback) {
    if (!s_terrain) return fallback;
    const float xy[2] = {x, y};
    float z = fHeight(s_terrain, xy);
    return std::isfinite(z) ? z : fallback;
}

// The toolkit's engine pointer alternates between (apparently) the world and the proposal engine, so the
// index is looked up again when it changes, but only logged when the value itself changes.
static int EdgeTypeIndex(void* engine) {
    static int s_loggedIdx = -2;
    if (engine != s_idxEngine) {
        void* ti = reinterpret_cast<void*>(g_t.baseEdgeTypeInfo);
        s_edgeIdx = fTypeIndex(static_cast<char*>(engine) + 0x48, &ti);
        s_idxEngine = engine;
        if (s_edgeIdx != s_loggedIdx) {
            Logf("cross: BaseEdge component index %d", s_edgeIdx);
            s_loggedIdx = s_edgeIdx;
        }
    }
    return s_edgeIdx;
}

// std::map<Entity, std::set<Entity>> (MSVC): node {left, parent, right, color, isnil@0x19, key@0x20,
// set{head@0x28, size@0x30}}.  Returns the node's edge count, -1 if not found.
static int NodeDegree(void* map, int key) {
    char* head = static_cast<char*>(pget(map, 0));
    char* n = static_cast<char*>(pget(head, 8));
    char* best = head;
    for (int guard = 0; guard < 64 && n && bget(n, 0x19) == 0; ++guard) {
        if (iget(n, 0x20) < key) n = static_cast<char*>(pget(n, 0x10));
        else { best = n; n = static_cast<char*>(pget(n, 0)); }
    }
    if (best == head || key < iget(best, 0x20)) return -1;
    return (int)*reinterpret_cast<uint64_t*>(best + 0x30);
}

static void CollectLines(void* engine, void* coneSys, void* node2seg, void* view, void* cursor) {
    s_lines.clear();
    struct { int32_t* b; int32_t* e; int32_t* cap; } vec = {nullptr, nullptr, nullptr};
    oCollect(&vec, engine, coneSys, view, cursor, kConePx * g_opt.scale.load(std::memory_order_relaxed), 15000.0f);
    if (!vec.b) return;
    const int idx = EdgeTypeIndex(engine);
    const uintptr_t masks = reinterpret_cast<uintptr_t>(pget(engine, 0xc0));
    if (idx < 0 || idx >= 128 || !masks) {
        CrossBroken("BaseEdge component index / mask table not sane");
    } else {
        std::vector<int32_t> ents(vec.b, vec.b + (std::min)((ptrdiff_t)kMaxEntities, vec.e - vec.b));
        std::sort(ents.begin(), ents.end());
        ents.erase(std::unique(ents.begin(), ents.end()), ents.end());
        for (int32_t ent : ents) {
            if (ent < 0) continue;
            uint64_t w = *reinterpret_cast<const uint64_t*>(masks + (uintptr_t)ent * 16 + (idx >> 6) * 8);
            if (!((w >> (idx & 63)) & 1)) continue;   // nodes, constructions ...
            char* c = static_cast<char*>(fComp(engine, ent, idx));   // BaseEdge (0x118)
            if (!c || !Readable(reinterpret_cast<uintptr_t>(c), 0x40)) continue;
            for (int k = 0; k < 2; ++k) {
                // a plain joint along a road (2 edges) is not an "end"
                if (NodeDegree(node2seg, iget(c, k * 4)) == 2) continue;
                const float* p = reinterpret_cast<const float*>(c + 8 + k * 12);
                const float* t = reinterpret_cast<const float*>(c + 0x20 + k * 12);
                float tl = std::hypot(t[0], t[1]);
                if (!(tl > 1e-3f)) continue;
                s_lines.push_back({p[0], p[1], p[2], t[0] / tl, t[1] / tl});
            }
        }
    }
    GameVecFree(vec.b, (uint64_t)((char*)vec.cap - (char*)vec.b));
}

struct Pick { bool ok; float x, y, d; Line line, farthest; };

// (cx, cy) point, dir unit vector it may slide along (null = along each road's direction), S start or
// null, tol metres.  Mirrors pickCross in snaptool.py (unit-tested there).
static Pick PickCross(float cx, float cy, const float* dir, const float* S, float tol) {
    Pick best{};
    const float cosMax = std::cos(kMaxDeg * 3.14159265f / 180.0f);
    float sx = 0, sy = 0;
    if (S) {
        float L = std::hypot(cx - S[0], cy - S[1]);
        if (L < 5.0f) return best;
        sx = (cx - S[0]) / L; sy = (cy - S[1]) / L;
    }
    for (const Line& l : s_lines) {
        float dist = (cx - l.px) * l.ux + (cy - l.py) * l.uy;   // signed distance from the cross line
        if (std::fabs(dist) > tol) continue;
        if (S && std::fabs((S[0] - l.px) * l.ux + (S[1] - l.py) * l.uy) < 4.0f) continue;   // through the start
        float nx, ny;
        if (dir) {
            float cu = dir[0] * l.ux + dir[1] * l.uy;
            if (std::fabs(cu) < cosMax) continue;
            float t = dist / cu;
            nx = cx - dir[0] * t; ny = cy - dir[1] * t;
        } else {
            if (std::fabs((cy - l.py) * l.ux - (cx - l.px) * l.uy) < 2.0f) continue;   // on the road's own axis
            nx = cx - l.ux * dist; ny = cy - l.uy * dist;
        }
        if (S && (nx - S[0]) * sx + (ny - S[1]) * sy < 4.0f) continue;   // would leave a stub < 4 m
        if (!best.ok || std::fabs(dist) < best.d) best = {true, nx, ny, std::fabs(dist), l, l};
    }
    if (!best.ok) return best;
    // draw from the farthest end node on the same cross line (several roads ending flush)
    float farD = -1;
    for (const Line& l : s_lines) {
        if (std::fabs(l.ux * best.line.ux + l.uy * best.line.uy) < 0.9998f) continue;
        if (std::fabs((best.x - l.px) * l.ux + (best.y - l.py) * l.uy) > 0.5f) continue;
        float dd = std::hypot(best.x - l.px, best.y - l.py);
        if (dd > farD) { farD = dd; best.farthest = l; }
    }
    return best;
}

// Position of SnapPoint `out` (an edge snap) with t replaced, computed by the game from (edge, t, offset).
static void EdgePos(void* engine, const void* out, float t, float xyz[3]) {
    alignas(16) char sp[0x40];
    memcpy(sp, out, 0x34);
    fset(sp, 4, t);
    bset(sp, 0x2c, 0);   // drop the cached position so it is recomputed
    float res[4] = {0, 0, 0, 0};
    fSnapPos(res, engine, sp, 0);
    xyz[0] = res[0]; xyz[1] = res[1]; xyz[2] = res[2];
}

static void ApplyCross(const CursorCall& x) {
    Snap result{};
    if (!g_opt.crossOk.load() || s_crossBroken || !on(g_opt.guideCross) ||
        !(on(g_opt.snapGuides) || on(g_opt.showGuides))) {
        SetSnap(result);
        return;
    }
    void* o = x.out;
    const int edge = iget(o, 0);
    const float t0 = fget(o, 4);
    const bool onTerrain = edge == -1 && bget(o, 0xd) == 1;
    const bool onOffset = edge >= 0 && fget(o, 8) != 0.0f && t0 > 0.001f && t0 < 0.999f;
    if ((!onTerrain && !onOffset) || !x.node2seg || !x.view || !x.cursor || !bget(x.cursor, 0x14)) {
        SetSnap(result);
        return;
    }
    void* engine = pget(x.toolkit, 0xb0);
    void* coneSys = pget(x.toolkit, 0xc0);
    s_terrain = pget(x.toolkit, 0xa8);
    if (!engine || !coneSys || !Readable(reinterpret_cast<uintptr_t>(x.node2seg), 16) ||
        !Readable(reinterpret_cast<uintptr_t>(pget(x.node2seg, 0)), 0x20)) {
        CrossBroken("toolkit / node2seg pointers not readable");
        SetSnap(result);
        return;
    }
    const uint64_t now = GetTickCount64();
    if (now - s_linesAt > kRefreshMs) {   // time only: the engine pointer alternates frame to frame
        CollectLines(engine, coneSys, x.node2seg, x.view, x.cursor);
        s_linesAt = now;
    }
    if (s_lines.empty()) { SetSnap(result); return; }

    float Sbuf[2];
    const float* S = nullptr;
    if (x.start) { Sbuf[0] = fget(x.start, 0); Sbuf[1] = fget(x.start, 4); S = Sbuf; }
    float C[3], dirBuf[2];
    const float* dir = nullptr;
    if (onTerrain) {
        C[0] = fget(o, 0x10); C[1] = fget(o, 0x14); C[2] = fget(o, 0x18);
        if (S) {
            float L = std::hypot(C[0] - S[0], C[1] - S[1]);
            if (L >= 5.0f) { dirBuf[0] = (C[0] - S[0]) / L; dirBuf[1] = (C[1] - S[1]) / L; dir = dirBuf; }
        }
    } else {
        EdgePos(engine, o, t0, C);
        const float h = t0 < 0.5f ? 0.002f : -0.002f;
        float C2[3];
        EdgePos(engine, o, t0 + h, C2);
        float ex = (C2[0] - C[0]) / h, ey = (C2[1] - C[1]) / h, el = std::hypot(ex, ey);
        if (!(el > 1e-6f)) { SetSnap(result); return; }
        dirBuf[0] = ex / el; dirBuf[1] = ey / el; dir = dirBuf;
    }
    const char* v = static_cast<const char*>(x.view);
    float cam = std::sqrt((C[0] - fget(v, 0x24)) * (C[0] - fget(v, 0x24)) +
                          (C[1] - fget(v, 0x28)) * (C[1] - fget(v, 0x28)) +
                          (C[2] - fget(v, 0x2c)) * (C[2] - fget(v, 0x2c)));
    float mpp = 2.0f * cam * std::tan(fget(v, 0x10) / 2.0f) / (float)(std::max)(1, (int)iget(v, 4));
    Pick r = PickCross(C[0], C[1], dir, S, kTolPx * g_opt.scale.load(std::memory_order_relaxed) * mpp);
    if (!r.ok) { SetSnap(result); return; }

    const bool snap = on(g_opt.snapGuides);   // off: only show where the alignment is
    float X[3];
    if (onOffset) {
        // stay on the offset line: solve (pos(t) - P) . u = 0 for t (Newton on the game's own pos(t))
        const Line& l = r.line;
        float t = t0, pos[3] = {C[0], C[1], C[2]};
        bool ok = false;
        for (int i = 0; i < 8; ++i) {
            float f = (pos[0] - l.px) * l.ux + (pos[1] - l.py) * l.uy;
            if (std::fabs(f) < 0.02f) { ok = true; break; }
            const float h = t < 0.5f ? 0.001f : -0.001f;
            float ph[3];
            EdgePos(engine, o, t + h, ph);
            float df = ((ph[0] - pos[0]) * l.ux + (ph[1] - pos[1]) * l.uy) / h;
            if (!(std::fabs(df) > 1e-6f)) break;
            t -= f / df;
            if (!(t > 0.001f && t < 0.999f)) break;   // the line is past this edge's end
            EdgePos(engine, o, t, pos);
        }
        if (!ok) { SetSnap(result); return; }
        X[0] = pos[0]; X[1] = pos[1]; X[2] = pos[2];
        if (snap) {
            fset(o, 4, t);
            if (bget(o, 0x2c)) {   // keep the cached position consistent (CalcCursorPoint fills it this way)
                fset(o, 0x20, pos[0]); fset(o, 0x24, pos[1]);
                fset(o, 0x28, TerrainZ(pos[0], pos[1], fget(o, 0x28)));
            }
        }
    } else {
        X[0] = r.x; X[1] = r.y;
        X[2] = TerrainZ(r.x, r.y, C[2]);
        if (std::fabs(X[2] - C[2]) > 300.0f) X[2] = C[2];
        if (snap) { fset(o, 0x10, X[0]); fset(o, 0x14, X[1]); fset(o, 0x18, X[2]); }
    }
    result = {true, X[0], X[1], X[2], r.farthest, now};
    SetSnap(result);
}

// Dashed guide from the end node to (a bit past) the aligned point: 6 m dash, 4 m gap, light blue.
static void DrawCross(void* builder, const Snap& s) {
    void* rend = pget(builder, 0x970);
    if (!rend) return;
    const Line& P = s.from;
    float vx = s.x - P.px, vy = s.y - P.py;
    const float L0 = std::hypot(vx, vy);
    if (L0 < 1.0f) return;
    vx /= L0; vy /= L0;
    const float L = L0 + 12.0f, dash = 6.0f, gap = 4.0f;
    alignas(16) static char edges[kMaxDash][0x40];
    int n = 0;
    for (float a = 0; a < L && n < kMaxDash; a += dash + gap) {
        float b = (std::min)(a + dash, L);
        if (b - a < 0.5f) break;
        float p0[4], p1[4];
        const float d[2] = {a, b};
        float* pts[2] = {p0, p1};
        for (int k = 0; k < 2; ++k) {
            float px = P.px + vx * d[k], py = P.py + vy * d[k];
            pts[k][0] = px; pts[k][1] = py;
            pts[k][2] = (std::max)(P.pz, TerrainZ(px, py, P.pz)) + 0.3f;
            pts[k][3] = 0;
        }
        fStraight(edges[n], p0, p1);
        ++n;
    }
    if (!n) return;
    struct { void* b; void* e; void* cap; } vec = {edges[0], edges[n], edges[n]};
    static const float colA[4] = {0.35f, 0.85f, 1.0f, 0.3f};    // occluded pass
    static const float colB[4] = {0.35f, 0.85f, 1.0f, 0.85f};   // visible pass
    fDraw(rend, &vec, 0.6f, colA, colB);
}

// 58a100 UpdateRenderer(builder, ..): after the builder has queued its own preview lines for the frame.
static void* hRenderer(void* builder, void* a1) {
    void* ret = oRenderer(builder, a1);
    if (!on(g_opt.guideCross) || !on(g_opt.showGuides) || s_crossBroken) return ret;
    Snap s;
    {
        std::lock_guard<std::mutex> lk(s_snapMx);
        s = s_snap;
    }
    if (!s.valid || GetTickCount64() - s.at > kSnapTtlMs) return ret;
    DrawCross(builder, s);
    return ret;
}

// =====================================================================================================

// ---- 585e50 StreetBuilder::Step: heartbeat for "the road / track tool is open" (panel visibility) ----
// virtual void Step(CRendererComponent*, float dt): rcx this, rdx renderer, xmm2 dt.  Roads and tracks are
// the same tool class and the game steps it once per frame while it is open (re/NOTES.md 7, 10, 14).  Declared as
// returning void* only so whatever is in RAX is handed back untouched, like every other detour here.
typedef void* (*StepFn)(void*, void*, float);
static StepFn oStep = nullptr;
static std::atomic<uint64_t> s_stepAt{0};
static void* hStep(void* self, void* renderer, float dt) {
    s_stepAt.store(GetTickCount64(), std::memory_order_relaxed);
    return oStep(self, renderer, dt);
}

bool BuilderToolOpen(uint64_t graceMs) {
    const uint64_t at = s_stepAt.load(std::memory_order_relaxed);
    return at && GetTickCount64() - at < graceMs;
}

static bool create(uintptr_t target, void* detour, void** orig, const char* name) {
    if (!target) { Logf("hook %s: no target", name); return false; }
    if (MH_CreateHook(reinterpret_cast<void*>(target), detour, orig) != MH_OK) {
        Logf("hook %s: MH_CreateHook failed", name);
        return false;
    }
    return true;
}

bool InstallHooks() {
    if (MH_Initialize() != MH_OK) { Logf("MH_Initialize failed"); return false; }
    oOpNew = reinterpret_cast<void* (*)(uint64_t)>(g_t.operatorNew);
    oOpDel = reinterpret_cast<void (*)(void*, uint64_t)>(g_t.operatorDelete);
    bool ok = true;
    ok &= create(g_t.calcCursorPoint, (void*)hCalc, (void**)&oCalc, "CalcCursorPoint");
    ok &= create(g_t.findSnapBaseEdge, (void*)hFind, (void**)&oFind, "FindSnapPointBaseEdge");
    ok &= create(g_t.myFindSnapPointNew, (void*)hMyFind, (void**)&oMyFind, "MyFindSnapPointNew");
    ok &= create(g_t.roadEarlyAbort, (void*)hRoadAbort, (void**)&oRoadAbort, "RoadEarlyAbort");
    ok &= create(g_t.trackNearScore, (void*)hTrackNear, (void**)&oTrackNear, "TrackNearScore");
    ok &= create(g_t.trackFarScore, (void*)hTrackFar, (void**)&oTrackFar, "TrackFarScore");
    ok &= create(g_t.collectNodesAndEdges, (void*)hCollect, (void**)&oCollect, "CollectNodesAndEdges");
    ok &= create(g_t.getSnapTangents, (void*)hTangents, (void**)&oTangents, "GetSnapTangents");
    if (!ok) { Logf("some hooks failed to create; enabling none"); MH_Uninitialize(); return false; }

    // optional: the cross guide (ResolveTargets already decided crossOk from the signatures)
    if (g_opt.crossOk.load()) {
        fTypeIndex = reinterpret_cast<TypeIndexFn>(g_t.getComponentTypeIndex);
        fComp = reinterpret_cast<CompFn>(g_t.componentData);
        fHeight = reinterpret_cast<HeightFn>(g_t.heightAt);
        fSnapPos = reinterpret_cast<SnapPosFn>(g_t.snapPointPos);
        fStraight = reinterpret_cast<StraightFn>(g_t.makeStraightEdge);
        fDraw = reinterpret_cast<DrawFn>(g_t.drawEdges);
        if (!create(g_t.updateRenderer, (void*)hRenderer, (void**)&oRenderer, "UpdateRenderer")) {
            g_opt.crossOk = false;
            Logf("cross guide off: UpdateRenderer hook failed");
        }
    }
    // optional: the panel shows only while the road / track tool is open
    const bool step = create(g_t.streetBuilderStep, (void*)hStep, (void**)&oStep, "StreetBuilderStep");
    if (!step) Logf("tool detection off: the panel ignores 'only with the road / track tool'");
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) { Logf("MH_EnableHook failed"); MH_Uninitialize(); return false; }
    g_opt.toolDetect = step;
    Logf("all hooks installed (cross guide %s, tool detection %s)", g_opt.crossOk.load() ? "on" : "off",
         step ? "on" : "off");
    return true;
}

void RemoveHooks() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

}  // namespace tf3
