# Starry crystal arena environment

All geometry, shaders and particles are original procedural assets. Presentation
consumes no gameplay RNG and reads no files during combat.

## Shared map integration (schema v2)

Build both src/environment/environment_renderer.cpp and environment_scene.cpp.
MapRenderer::loadMap enables this preset only when Map::getEnvironmentPreset()
is EnvironmentPreset::StarryCrystal. A material name cannot activate encounters
or switch off the environment. Legacy maps default to None. A starry scene with
no floor draws only its sky.

deriveEnvironmentScene(map) creates a rendering snapshot from the same
Map::getArenaLayout() used by gameplay: main-floor bounds/elevation and explicit
rift slots, positions, rotations and scales. It never rediscovers a floor or
guesses cardinal positions. Solid obstacles are the actual Map::getBrushes()
vertices consumed by collision. Crystal motifs normalize those actual brush
bounds, including after moving/resizing. Static lights use Map::getLights(),
already resolved from shared light anchors; no material-based light generator
or duplicate light coordinates exist in this renderer.

After a successful map edit transaction, call MapRenderer::loadMap(map) to upload
new geometry and rebuild the derived snapshot/lights. Map mutation alone does
not update already-uploaded GL buffers. This is a load boundary, not per-frame
work. Destroy MapRenderer while its GL context is current.

Before each rendered frame:
~~~cpp
mapRenderer->setEnvironmentTime(combatSeconds);
mapRenderer->setRiftWarnings({slot0Progress, slot1Progress, slot2Progress, slot3Progress});
~~~

Warnings are -1 inactive or 0..1 progress. Slots are authored identity, independent
of world compass direction. Freeze both inputs while paused. resetEnvironment()
resets time/warnings and preserves particle count. Initial particle count comes
from Map::getEnvironment().particleCount. The pressure override remains
setEnvironmentParticleCount(2000), capped at 2000.

Map::getEnvironment().transformationStart and transformationDuration drive the
one smooth transition; App audio reads the same start value. No geometry or
collision changes during the transition. Rifts apply authored XYZ Euler rotation
(X then Y then Z), positive scale, then translation. Ground rings, hollow upright
apertures and warm warning lights share that transform. Particle distribution and
peripheral scenery follow the shared main-floor bounds/elevation.

The preset retains its motif frequencies, 76 distant silhouettes and colors as
art choices, rather than authored world coordinates. Four rift warning slots
remain the encounter contract; arbitrary enemy-wave cardinality is out of scope.

## Authoring and compatibility

The canonical v2 map is authored through the shared Map module/editor. The
historical sample generator is only an archival v1 mesh export:

~~~sh
python3 src/environment/generate_arena.py --output /tmp/historical-arena.json
~~~

An explicit output is required, and the canonical arena path is rejected. This
generator must not overwrite the migrated v2 document.

The original sample has 36x36 floor bounds, the south start, one central crystal
cluster and two side covers. Its actual coordinates, dimensions, entity transforms
and anchored lights now live only in the map. Top crown recesses remain enclosed
by the pre-existing conservative AABB collision; this upgrade does not alter
projectile or player collision rules.

## Validation

The schema-v2 upgrade passed C++17 syntax checking, offline glslang linking of all
four shader pairs, and CPU verification of configurable transition timing,
translated floor/backdrop, non-cardinal rift transforms/nonuniform scale, stable
warning slot identity and deterministic geometry. The historical generator was
verified to reject absent output and the canonical-map target without changing
the canonical file hash.

Run the pure transform checks without a graphics context:
~~~sh
clang++ -std=c++17 -O2 -Iinclude src/environment/environment_scene.cpp src/environment/validation/scene_math_test.cpp -o /tmp/silic2_environment_math_test
/tmp/silic2_environment_math_test
~~~

Run shared Map integration checks after the new Map implementation is built:
~~~sh
clang++ -std=c++17 -O2 -Iinclude src/environment/environment_scene.cpp src/environment/validation/map_scene_test.cpp src/engine/map.cpp src/engine/simple_json.cpp -o /tmp/silic2_environment_map_test
/tmp/silic2_environment_map_test
~~~

The shared-Map integration probe passed. It checks a plain-material floor with explicit starry preset,
moved/resized geometry and anchored lights, moved/scaled rifts, resized floor
bounds, sky-only maps, and legacy None. On 2026-10-07 an ATLAS-coordinated hidden OpenGL run also passed: plain-floor
material leaves the environment enabled; moved/resized central geometry and a
rift moved to (12,0,6), yaw 35 degrees and scale (2,1.5,.8) render from the shared
map; its warm warning light resolves to (12,1.5,6). Sky-only loading, legacy map
switching, reset/frozen pixel comparisons and star translation invariance passed
with GL error 0. The window exited and GPU use was handed back to the integrator.
Screenshots and the log are retained in validation/artifacts/map-v2/. The baseline
uses a 320x200 internal framebuffer and 1280x720 actual Retina output. These are
module integration checks, not a new full-game playtest or performance acceptance.

The original sample's hidden-GL checks on Apple M2 passed shader linking, arena /
legacy map switching, frozen/reset framebuffer comparisons and translation-
invariant sky. Its 320x200 scene and 1280x720 Retina postprocess were inspected.
Original 2000-particle isolated glFinish probes measured p95 0.518..1.343 ms,
p99 1.560..3.356 ms without isolating concurrent workspace activity; these are
historical module measurements, not full-run performance acceptance.

The optional macOS GPU probe is deliberately outside application sources in
validation/preview.cpp. Coordinate GPU use before running it:
~~~sh
clang -Iinclude -c src/engine/glad.c -o /tmp/silic2_env_glad.o
clang++ -std=c++17 -O2 -Iinclude src/environment/validation/preview.cpp src/environment/environment_renderer.cpp src/environment/environment_scene.cpp src/engine/map_renderer.cpp src/engine/map.cpp src/engine/simple_json.cpp src/engine/shader.cpp src/engine/texture.cpp src/engine/pixel_renderer.cpp /tmp/silic2_env_glad.o -L/opt/homebrew/lib -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -o /tmp/silic2_env_preview
/tmp/silic2_env_preview
~~~

It writes PPM screenshots under /tmp/silic2_env_*.ppm. Convert with sips if needed.
Windows/Linux runtime checks and human playtests remain separate acceptance work.
