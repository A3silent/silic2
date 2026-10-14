# Crystal texture acceptance — 2026-10-13

NEBULA generated and integrated the four requested surface families. Asset
provenance and runtime settings are in
[`res/texture/crystal_combat`](../../../res/texture/crystal_combat/README.md).
Evidence root: `artifacts/acceptance/crystal-textures/` (workspace artifacts).

## Visual evidence

- `before/` contains real GL captures compiled against the saved pre-pass renderer
  and shader; `after/` repeats the same combat preview cameras and times.
- `checks/*-sampled[-320].png` shows each material at output/native resolution.
- `checks/*-gpu-flat-control.png` temporarily substitutes mean-color **GPU data**
  for exactly one material, keeping all other uniforms and shading fixed. Each
  material changes pixels; restoring original GPU bytes restores exact pixels.
  These controls are diagnostics, not generated or delivered artwork.
- `checks/actors-before.png` uses the current shader with all four profile bits
  disabled; `actors-after.png` enables them at the same time/camera. These use
  actual CharacterPresentation Chaser, charging Sentry, hands and weapon with
  synthetic fixed poses. They are not a gameplay or human playtest recording.
- `checks/actors-after-shift.png` shows time 49; environment `after/` also includes
  time 0/47/49, bridge underside, ramps, leaning supports and warm upper rift.
- `*-repeat.png` shows each unmodified runtime tile repeated 3×3 for inspection;
  `runtime-contact-sheet.png` compares the four tiles. No strict seamless claim.

NEBULA inspected native 320×200 material views, arrival before/after, fixed actors,
shifted actors and warm rift. Ceramic now has broad mineral patches; crystal has
surface cleavage with reduced old bands. Metal seams remain subdued; dark shell
stays dark. Warm actor/rift accents remain prominent in these fixed views.
ATLAS separately reviewed originals, repeats and actor comparison and requested
freezing current strength without further detail expansion.

## Checks and results

`checks/validation.log` and `checks/sampling.jsonl`: real GL 64×64 upload, actual
sampling of all four materials, independent mask control and exact GPU restoration.
Changed output pixels against flat GPU control (of 921,600): metal 116,188;
ceramic 97,228; shell 218,324; crystal 187,896.

All four runtime files were temporarily renamed with exception-safe restoration,
the texture cache cleared, and the map reloaded. Missing-file diagnostics were
expected. The resulting fallback exactly matched the disabled-texture frame.
Original/runtime files were restored; normal reload passed. Repeated frozen frames
and serialized Map equality passed.

`after/validation.log`: timed transformation, freeze/reset, retired observation
no-op, cross-theme return and immutable Map passed. `regression-summary.json`:
all 68 protected source/assets retain their pre-pass hashes; three Hollow views
at 0/47/49 match the before PPM bytes exactly. Geometry remains 23 model instances,
24 brushes, 1,152 detail vertices and 32,256-byte detail VBO. Scene draw count
remains 27 including sky/details/particles; no new geometry or rendering pass.

`checks/paired-render-times.csv` alternates masks 0/15, same fixed actors/camera,
20 warmup samples discarded per mask, then 140 measurements each. Timings include
CPU render submission and `glFinish`, exclude capture writes. Disabled/enabled
median: 0.8854/0.8927 ms; p95: 0.9667/0.9589 ms; p99: 1.0487/1.0346 ms. This short
local diagnostic shows no material regression; it is not a full-game frame budget
or sustained combat gate. Shader cost is three texture fetches per textured
fragment; logical RGB mip data totals 65,532 bytes, physical GPU allocation unknown.

Normal Debug and Release application rebuilds passed, no warnings in these build
logs. Executables `silic2` and `silic2-release` are ready to run from repo root.
Makefile, gameplay, maps, OBJ meshes, actor assets, audio and Hollow were not edited
by this pass. No long combat test or new human playtest was requested/performed.

## Reproduction

From repo root, with an ATLAS-assigned exclusive GPU window:

```sh
clang -Iinclude -c src/engine/glad.c -o /tmp/silic2_crystal_glad.o
clang++ -std=c++17 -O2 -Iinclude src/environment/validation/texture_preview.cpp src/character/character_presentation.cpp src/environment/environment_renderer.cpp src/environment/environment_scene.cpp src/engine/map_renderer.cpp src/engine/map.cpp src/engine/simple_json.cpp src/engine/mesh_asset.cpp src/engine/spatial_world.cpp src/engine/shader.cpp src/engine/texture.cpp src/engine/pixel_renderer.cpp /tmp/silic2_crystal_glad.o -L/opt/homebrew/lib -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -o /tmp/silic2_texture_check
mkdir -p artifacts/acceptance/crystal-textures/checks
/tmp/silic2_texture_check artifacts/acceptance/crystal-textures/checks
make -f Makefile.map -j2
make -f Makefile.map BUILD=release -j2
```

The file-missing check temporarily renames runtime PNGs: do not run the app or
another probe concurrently. The original generated images are never renamed.
Build `combat_preview.cpp` with the command in `COMBAT.md` to repeat the fixed
scene regression, passing `artifacts/acceptance/crystal-textures/after` as output.
PPM-to-PNG conversion is lossless. Both probes release GL resources/cache before
closing their contexts. The acceptance GPU window has been released.
