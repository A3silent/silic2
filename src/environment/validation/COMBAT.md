# Crystal combat environment — 2026-10-13

Implements NEBULA's portion of `plan/crystal-combat-world.md`. The selected Crystal
map returns to 90-second arena survival; it has no observation or exit objective.
VECTOR owns game rules, spatial queries, player/enemy integration and performance
acceptance. PRISM owns the same-format editor. This record is environment evidence.

## Authored space

A real curved upper walk (6m high, 6m wide), two 15m-long/6m-rise slopes, lower
crystal vault, leaning-support court and bright eastern passage connect around a
central irregular shell. The reference foundation is 64×64, while the playable
visible perimeter is a closed curved shell of inner diameter 56. Perimeter heights
vary from 10.15 to 16.1m. Normal jump tuning gives 1.6m of net rise above the 6m walk;
actual outward jump containment remains part of VECTOR's player verification.

Slopes meet the bridge at identical Y/Z, without overlapping a hidden riser.
They were shortened after shared queries showed a long slope pinched the southern
bypass against the curved wall. Navigation was reduced from a dense authoring grid
to 85 explicit nodes / 171 links while retaining curve, slope and turning nodes.
The route has 41 points. All 40 segments pass the shared `canTraverse` query at
radius .65; zero map diagnostics is a query gate, not a substitute for actual input
movement or enemy pursuit.

Seven original reusable OBJ modules and deterministic authoring tools provide
23 model instances. The first map uses identical visual and collision meshes:
ramps are slopes, bridge undersides are empty, the vault is open and supports
are inclined geometry. It does not substitute instance bounds for collision.
The shared Map compiles model instances into brushes consumed by MapRenderer;
the environment renderer neither parses model files nor owns physical geometry.

## Presentation isolation

`environment.architecture="crystal_combat"` selects the new style independently
of game mode. The new style reads authored region and landmark data, keeps rift
transforms/slots authoritative, and uses the existing 45s/4s time transformation.
The retired observation state API is a no-op for this style. Historical exploration
rendering remains available for compatibility fixtures, without affecting battle.

The physical models use broad stone/metal planes, directional crystal clefts,
upper-deck inlays and restrained regional contrast. Two incomplete distant orbital
bands sit outside the floor. The sky carries a large dark celestial body. No
collision moves during transformation, and no new texture, framebuffer, transparent
layer or per-frame mesh upload was introduced. Hollow map/shaders, shared pixel
postprocess and all actor/weapon/audio assets remain untouched by NEBULA.

Current scene: 24 brushes + sky/details/points = 27 scene draw calls; 870 collision
triangles; 1,152 detail vertices / 32,256-byte static detail VBO; 128 particles.
Full-game performance is measured separately by VECTOR with actors and audio.

## Validation and evidence

`combat_map_check.cpp` uses public Map/SpatialWorld queries and the derived render
snapshot. `combat_preview.cpp` creates a real hidden GLFW/OpenGL context and writes
320×200 raw scene and 1280×720 postprocessed framebuffer images. Normal composition
views use 45° FOV, overview uses 65°. Camera placement is a render probe, not a claim
of player movement or human playtesting.

Evidence lives in `artifacts/acceptance/crystal-combat/env/`. The first pass includes
all five areas, bridge above/below, slope, upper-rift warning, time 0/47/49, repeated
frozen frames, reset equality, ignored observation state, immutable serialized Map
and same-process return from Hollow. Exact Hollow views at 0/47/49 are compared
byte-for-byte against the dual-arena baseline. NEBULA inspected actual images;
ATLAS reviewed the five-region composition and accepted the curved bridge, real
underpass and leaning supports for this prototype. Terrain and assets are frozen.
The final horizontal arrival capture is `00-arrival-horizontal.png`; GL exited and
the GPU window was returned before the handoff. Low-level combat readability and
full performance remain VECTOR integration checks; human playtesting is pending.

The final content version is `crystal-combat-world-v1`. Only that metadata value
changed after the final GL capture; geometry, materials, lights and shaders are
identical. `final-input-hashes.json` records the frozen map, assets, authoring tools
and environment implementation. `manifest.json` retains the capture-input hash
and separately records the final map hash so this metadata update is explicit.

Build CPU check:

```sh
clang++ -std=c++17 -O2 -Iinclude src/environment/validation/combat_map_check.cpp src/environment/environment_scene.cpp src/engine/map.cpp src/engine/simple_json.cpp src/engine/mesh_asset.cpp src/engine/spatial_world.cpp -o /tmp/silic2_combat_map_check
/tmp/silic2_combat_map_check
```

Build/run preview only in an ATLAS-assigned GPU window:

```sh
clang -Iinclude -c src/engine/glad.c -o /tmp/silic2_crystal_glad.o
clang++ -std=c++17 -O2 -Iinclude src/environment/validation/combat_preview.cpp src/environment/environment_renderer.cpp src/environment/environment_scene.cpp src/engine/map_renderer.cpp src/engine/map.cpp src/engine/simple_json.cpp src/engine/mesh_asset.cpp src/engine/spatial_world.cpp src/engine/shader.cpp src/engine/texture.cpp src/engine/pixel_renderer.cpp /tmp/silic2_crystal_glad.o -L/opt/homebrew/lib -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -o /tmp/silic2_combat_preview
/tmp/silic2_combat_preview artifacts/acceptance/crystal-combat/env
```
