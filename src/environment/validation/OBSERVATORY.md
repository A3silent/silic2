# Crystal Observatory environment — 2026-10-13

Implements the environment portion of `plan/crystal-observatory.md`. Map data,
physical walls, gameplay progression and application integration belong to VECTOR.
This document describes environment evidence, not a completed human playtest.

## Scene and state

The renderer derives its regions, route, observation eye, focal point, fragment
depths, radius, landmark transform and exit from `Map::getExploration()` only when
both `GameMode::Exploration` and `EnvironmentPreset::StarryCrystal` are selected.
It does not infer gameplay from a material or filename.

The route contrasts a low brown entry, pale salt court, ink crystal gallery,
observation pocket and suspended optical vessel. Broad facets and sparse fractures
replace repeated crystal columns. A broken meridian above the court's north
parapet gives its opening a distinct silhouette. High gallery fins frame the sky;
the central vessel has missing facets, ivory edges and an incomplete armillary.
All low solid structures come from Map brushes. Decorative structures are high
above the walkable surface or outside its boundary; none adds an invisible wall.

Three annular sectors are actual static triangles at distinct authored depths.
Each center lies on the observation-eye/focal-point ray; its radius and width
scale with depth, so the three sectors project onto the same circle from the
authored eye. The near sector is the upper third to preserve overhead clearance.
Offset cameras see the sectors separate. This is not a HUD circle or billboard.

`MapRenderer::setObservationState(float progress, bool complete)` accepts the
controller snapshot. Progress brightens the fragments and observation marker;
completion reveals the vessel's internal directional star pattern, changes the
sky tint and brightens the route/exit markings. This is an artistic surface
approximation, not a portal, mirror or volumetric refraction. The renderer owns
no completion rule. Reset and map reload clear the visual state; a non-observatory
map ignores this API. Simulation time controls particles and all animation.

## Theme isolation and resources

No changes to Hollow shaders/map, common pixel postprocess, actors, weapons or
audio. The previous Crystal rendering branch remains available for non-exploration
Crystal maps. The new geometry is uploaded once; no runtime mesh upload, texture,
framebuffer, transparent veil or gameplay RNG was added. There is one detail
batch and the existing sky/point batches. Region uniforms are capped at eight and
route vertices at sixteen; current authored data fit those bounds. Contact shading
retains the sixteen-obstacle limit.

Final resource counts and capture inventory are in
`artifacts/acceptance/crystal-world/env/manifest.json`. Full-game frame-time
measurement, actors/HUD composition and actual progression are VECTOR's checks.

## Evidence

`artifacts/acceptance/crystal-world/env/` contains actual hidden GLFW/OpenGL
captures. `*-320.png` files capture the 320×200 scene framebuffer before the
unchanged pixel postprocess; other PNGs capture the actual 1280×720 window output.
Most composition views use 65 degrees; `09-player-fov-*` uses 45 degrees.

The probe checks shader linking/GL errors, all spatial nodes, a physically offset
observation view, partial progress, calibrated output, repeated-frame equality,
reset equality, cross-theme return, unchanged serialized Map and deterministic
geometry. It also projects every ring vertex to the common focal plane and checks
its radius to 0.0001 world units, with overhead clearance above three units.

`hollow-pixel-comparison.json` compares actual identical-camera frames at 0, 47
and 49 seconds to the prior dual-arena baseline, byte for byte. Calling the new
state API on Hollow is also checked to leave its pixels unchanged.

The first pass was visually inspected. Follow-up changes reduced block noise and
repeating diagonal stripes, raised selected vessel facets, and opened the court
skyline with a low parapet and broken meridian. ATLAS reviewed the final court skyline, route-facing court view and 45-degree
aligned observation view, accepted the composition for this playable prototype,
and froze CRYSTAL-ART. Human exploration and readability assessment remain pending;
no claim of human enjoyment or playability follows from these render probes.

## Reproduce only during an allocated GPU window

```sh
clang -Iinclude -c src/engine/glad.c -o /tmp/silic2_crystal_glad.o
clang++ -std=c++17 -O2 -Iinclude src/environment/validation/observatory_preview.cpp src/environment/environment_renderer.cpp src/environment/environment_scene.cpp src/engine/map_renderer.cpp src/engine/map.cpp src/engine/simple_json.cpp src/engine/shader.cpp src/engine/texture.cpp src/engine/pixel_renderer.cpp /tmp/silic2_crystal_glad.o -L/opt/homebrew/lib -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -o /tmp/silic2_observatory_preview
/tmp/silic2_observatory_preview artifacts/acceptance/crystal-world/env
```
