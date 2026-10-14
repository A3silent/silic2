# Orbital-organism environment pass — 2026-10-08

Historical horror-pass record. The subsequent [dual-map selection](DUAL.md)
assigns these unchanged visuals to HollowOrbit and restores StarryCrystal.

Implements plan/cosmic-horror-art-pass.md using original procedural geometry and
GLSL. The serialized StarryCrystal/environment preset names remain compatible;
map JSON, compiled obstacle meshes, rift transforms, light anchors and timings
are unchanged.

## Visible changes

- Fourteen enormous broken rib frames replace the decorative crystal ring.
  Their beams, offset spurs and suspended braces remain outside the authoritative
  floor rectangle. Rectangle-normal setback also handles very narrow/long maps.
- Central authored crystal mesh reads as ossified tissue: broad calcification,
  irregular interrupted seams and black clefts. Sparse dirty bone reflections
  clarify critical faces; slow surface contractions never move collision geometry.
- Mineral/oxidized floor plates and cover shoulders replace blue/cyan surfaces.
  Contact shading comes from the shared obstacle bounds. Worn shoulder reflections
  separate covers from the dark floor.
- Rifts retain stable slots and authored transforms but become asymmetric broken
  hollow apertures. Amber/vermilion floor warnings remain clearly separated from
  dormant bone/green surfaces. There is no opaque veil in the aperture.
- A sparse, dark sky contains a huge occluded celestial mass. The existing timed
  transition reveals an incompatible internal lattice and a few calcareous
  hanging supports, without whiteout, forced shake or geometry displacement.
- Ambient motes become dim, falling dust. Existing warm dynamic lights stay warm;
  only static environmental light colors are mapped to the preset's muted palette.
  Their authored positions, ranges, intensities and anchors are preserved.

Pixel postprocessing is deliberately unchanged: current shadow-preserving palette
quantization and gentle multiplicative CRT remain preferable to added noise or
global exposure reduction.

## Limits and integration

No public API or build source additions are needed. Rebuild for environment_scene
and map_renderer changes. All saved maps retain the explicit preset/mode split.
Non-environment legacy maps keep their existing renderer.

Default sample environment detail mesh: 5,832 vertices / 1,944 triangles, one
static VBO of 163,296 bytes. The default scene still uses 11 scene draw calls:
8 authored brushes, sky, static detail batch, particle batch. Particle cap is
unchanged at 2,000. No textures, offscreen passes, transparent layers, per-frame
mesh uploads, ray marching or gameplay RNG were added.

The map fragment shader keeps the existing maximum 128 point lights; an additional
floor-only loop considers at most 16 authored obstacle rectangles for contact
separation (7 in the default sample). These are cosmetic bounds, not another
collision or navigation representation. Noise is analytic and bounded; final
full-game performance remains VECTOR's integration check.

## Actual validation

Evidence: artifacts/acceptance/horror/env/.

- baseline/ contains the old presentation, captured before implementation using
  the same camera and renderer.
- phase-before.png, phase-mid.png, phase-after.png: same camera (0,1.6,13), looking
  at (0,3,0), 65-degree FOV; map-derived times 0,47,49.
- spawn-before.png and spawn-after.png: actual normal 45-degree FOV and horizontal
  view. layout.png verifies environment placement; rift-warning.png shows a warm
  warning and visible cover shoulder.
- validation.log records the hidden real OpenGL run. Internal framebuffer 320x200,
  actual Retina output 1280x720. Shader load/link, zero GL errors, identical frozen
  frames, reset pixel equality, unchanged serialized Map, legacy loading and
  returning to the environment all passed.
- Final spawn center ROI (x33..66%, y30..73%) display luminance median 10,
  p95 51.0128, maximum 85.5888 on a 0..255 scale. First-pass p95 was 27.28:
  the second pass raised selected bone reflections, while median remained 10.
- CPU scene_math_test and map_scene_test pass. The former now checks every
  peripheral vertex against default, narrow/long and large authored rectangles,
  plus translation, nonuniform rift scale/rotation and deterministic mesh output.
- Four shader pairs were linked offline with glslangValidator. Actual GPU runs
  were serialized through ATLAS and both processes exited; GPU has been released.

Visual inspection: floor perspective, central obstacle silhouette, cover shoulders
and warm warnings remain identifiable, with large black sky regions and selective
bone highlights. This is an environment-only judgment; human readability, combined
enemy/weapon/UI composition and full-game performance remain integration/playtest
work. No full-game performance run was made by NEBULA.

Reproduce the actual GL captures only during an allocated GPU window:
~~~sh
clang -Iinclude -c src/engine/glad.c -o /tmp/silic2_horror_glad.o
clang++ -std=c++17 -O2 -Iinclude src/environment/validation/horror_preview.cpp src/environment/environment_renderer.cpp src/environment/environment_scene.cpp src/engine/map_renderer.cpp src/engine/map.cpp src/engine/simple_json.cpp src/engine/shader.cpp src/engine/texture.cpp src/engine/pixel_renderer.cpp /tmp/silic2_horror_glad.o -L/opt/homebrew/lib -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -o /tmp/silic2_horror_preview
/tmp/silic2_horror_preview artifacts/acceptance/horror/env
~~~

The probe writes actual PPM framebuffer captures; PNGs are lossless sips conversions.
It is outside application sources and has no enemy/gameplay simulation.
