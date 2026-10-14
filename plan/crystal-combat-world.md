# Crystal combat world — confirmed implementation

User confirmed implementation after grilling on 2026-10-13. This plan supersedes
Crystal Observatory progression/layout in `crystal-observatory.md`. Preserve prior
uncommitted work; this is an incremental change. No commit/push requested.

## Accepted experience

Crystal only: one continuously contested arena with 4–5 interconnected regions,
short branches, real ramps and two overlapping walkable levels. Main routes form
loops with multiple entrances; small player jump/drop shortcuts have enemy detours.
Combat choices and discovering/remembering space are equally important. Preserve
full minimap, 90-second enemy generation then cleanup, existing weapon/enemies,
movement tuning and controls. No observation puzzle or exit-based win condition.
Side routes reward position, shortcuts and views; no new pickups or abilities.
Pressure varies between hotspots and short breathing routes, without permanently
safe upper platforms. Environment shifts are visual only, triggered by combat time.

Theme: crystal-invaded orbital megastructure mixed with architecture of unclear
origin. Five connected regions: central broken-ring court; lower crystal-shell
ravine; upper curved broken-ring walk; leaning-support side court; open-sky side
passage. Recognizable silhouette, light contrast, recurring central landmark.
Dimensions follow greybox movement/contact evidence rather than area alone.

## Deliverables and technical invariants

1. Shared map geometry supports rotated structures, ramps/curves and reusable
   static model instances. Separate visual mesh, collision proxy and walkable
   regions/connections. Keep legacy formats load/save compatible and reject bad
   resources/transforms transactionally. Relative assets resolve consistently in
   game and editor, including saved maps outside repository root.
2. A small shared spatial-query module hides actual surface intersection, ground
   support and broad phase. Player, weapons, enemy projectiles and line of sight
   agree on openings. Slopes support up/down/crest movement and sliding without
   changing movement speed/acceleration/jump/slide timing values. Multi-level
   navigation distinguishes overlapping XZ, supports ramp links and enemy detours.
   New spatial behavior is explicitly selected by new map data; Hollow and legacy
   behavior remain a regression baseline.
3. Hybrid authoring: editor makes and transforms primitive structures and places
   modeled assets; NEBULA supplies original reusable modeled modules. Implement
   practical static mesh import, transforms, duplication/snapping and visual,
   collision and navigation views. Support authored navigation/spawn adjustment
   and diagnostics; preserve undo/redo, save/reload and external-path usability.
   A complete DCC vertex editor, animated models and dynamic topology are out of scope.
4. Five-region battle map is genuinely playable, not render-only terrain. Maintain
   readable solid boundaries; main routes need no precise jumps. New map restores
   arena survival; decouple architectural presentation from exploration mode.
   Retain prior exploration test fixtures as history/compatibility when useful.
5. Inspect actual 320x200 and normal-window integrated scenes. Broad facets and
   strong silhouettes before detail; protect enemy/attack readability. Compare
   Hollow fixed views, preserve its default entry/assets and all actor/audio art.

## Ownership and handoff

ATLAS owns plan/docs/README/AGENTS and acceptance. VECTOR owns shared Map/asset/
collision/navigation/gameplay/App/HUD data/Makefile/tests. VECTOR may modify
player/enemy/weapon queries as needed for real geometry, preserving combat tuning.
NEBULA owns res/maps/starry_crystal_arena.json after shared schema contract, new
res/models/crystal_combat assets and asset generator, environment/map_renderer,
Crystal shaders and preview probes. PRISM owns tools/map_editor/** and its editor
self-tests/docs. Main build belongs VECTOR, editor CMake belongs PRISM.
Character/weapon art, audio, Hollow JSON/shaders and shared postprocess are read-only.
Publish the concrete shared schema/interface before dependent map/editor edits.
Cross-owner messages may coordinate interfaces; each file has one writer.

## Gates and completion

First build a geometry fixture: slope ascent/descent/crest/slide, curved passage
and projectile opening, bridge upper/lower traversal and enemy pursuit. Tests cross
normal public queries; no test-only collision or forced navigation.
Then author greybox, verify loops and enemy reachability for all zones/heights,
90s generation/cleanup and restart. Add art after spatial checks; editor can load,
edit/place/rotate/scale/duplicate, save/undo/redo/import this same data.
Run Debug/Release CPU tests, editor CTest and actual UI verification; actual GL
journey using normal PlayerInput/physics and ordinary shooting, integrated phase
screenshots, failure/pause/restart and legacy/Hollow flow/fixed-camera regression.
Measure final actual GL/audio >=90s including upper/lower busy views with raw frame
CSV: p95<=20ms, p99<=33.3ms, no 3 consecutive >50ms. Preserve all measured spikes
and distinguish synthetic validation from human playtest. Serialize GPU workloads.
ATLAS reviews real imagery and independently verifies evidence/hashes. Complete
with playable Crystal battle map, usable authoring workflow, documentation and
honest limitations. Human fun/readability is pending actual human playtest.
