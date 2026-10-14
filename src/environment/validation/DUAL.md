# Dual arena environments — 2026-10-08

The user requested retaining the new horror arena and all creature changes while
also restoring the original crystal arena. This is map-selected presentation,
not a global rollback.

## Implementation

- EnvironmentPreset::HollowOrbit loads the existing environment_* shaders and
  the current broken-rib geometry. The horror material, sky, rifts, illumination
  and falling dust are unchanged.
- EnvironmentPreset::StarryCrystal loads the five new starry_crystal_* shaders.
  Each is byte-identical to its original shader at ed76aee. The original
  procedural crystal geometry and static-light phase mapping are restored.
- Both use one EnvironmentRenderer/MapRenderer ownership and lifecycle. Clearing
  or loading a map releases the active theme's GL resources and cached lights.
  There is no second running scene or global theme switch.
- Floor bounds, authored obstacle meshes, rift slots/transforms, light anchors,
  transition timing and particle count still derive from the shared Map.
  None still selects the existing non-environment legacy rendering.
- No enemy, character, weapon, audio, HUD, player or gameplay implementation was
  changed by NEBULA. Public Map enum/JSON integration and the two authored map
  files belong to VECTOR.

No new application source list entries or public frame-update API are required.
Ship the five additional starry_crystal_* shader assets. The old environment_*
shader names now refer to HollowOrbit; shared vertex shaders remain reused.

## Resource bounds

| Theme | Detail vertices | Triangles | Static detail VBO |
| --- | ---: | ---: | ---: |
| HollowOrbit | 5,832 | 1,944 | 163,296 bytes |
| StarryCrystal | 3,804 | 1,268 | 106,512 bytes |

Only the selected theme is allocated. Default scenes retain 11 scene draw calls
(8 authored brushes, sky, details, particles) and the 2,000 particle limit. No
texture, framebuffer, transparent layer or per-frame mesh upload was added.

## Verification

Evidence is under artifacts/acceptance/dual-arena/env/:

- hollow/ and crystal/: seven actual PNGs each, including identical-camera
  transition times 0/47/49, normal 45-degree spawn views, rift warning and overview.
- pixel-comparison.json: all 14 framebuffer PPM comparisons are byte-identical.
  HollowOrbit matches artifacts/acceptance/horror/env/ final evidence.
  StarryCrystal matches artifacts/acceptance/horror/env/baseline/, captured before
  the horror pass using the ed76aee presentation.
- Each validation.log confirms actual OpenGL shader linking/rendering, frozen
  pixel equality, reset pixel equality, unchanged serialized input Map, legacy
  round-trip and same-process cross-theme return with pixel equality. Output is
  1280x720 Retina from a 320x200 internal framebuffer.
- dual_scene_test verifies both presets, geometry budgets, shared map transforms,
  editable rifts, deterministic generation, custom timings and None.
- scene_math_test and map_scene_test also passed. New shader pairs passed offline
  glslang linking. All five restored shader sources match ed76aee byte-for-byte.

The two GPU processes exited and GPU ownership was explicitly returned to VECTOR.
No full-game performance or large restart campaign was repeated. Combined
enemy/UI/audio checks remain integration work; screenshots are not human playtests.

## Reproduction

CPU:
~~~sh
clang++ -std=c++17 -O2 -Iinclude src/environment/environment_scene.cpp src/environment/validation/dual_scene_test.cpp src/engine/map.cpp src/engine/simple_json.cpp -o /tmp/silic2_dual_scene_test
/tmp/silic2_dual_scene_test
~~~

Use the existing compilation command in HORROR.md for the shared GL probe, then
during an allocated GPU window:
~~~sh
/tmp/silic2_dual_preview artifacts/acceptance/dual-arena/env/hollow res/maps/hollow_orbit_arena.json
/tmp/silic2_dual_preview artifacts/acceptance/dual-arena/env/crystal res/maps/starry_crystal_arena.json
~~~

The executable is built from validation/horror_preview.cpp, whose optional second
argument now selects the input map. It checks switching to the opposite theme and
back in the same GL context. PNGs are lossless conversions of its actual PPM output.
