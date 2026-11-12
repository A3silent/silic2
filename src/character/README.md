# KINETIC character presentation

Original procedural eldritch bone/tissue creatures and worn mechanical view model
for the accepted sample, revised by `plan/cosmic-horror-art-pass.md`. The current default is the original code-generated Crimson Carapace rifle;
the older optional Tripo adapter remains explicit compatibility, described below.
Enemy geometry and gameplay abilities are unchanged.

## Integration

Build `src/character/{character_presentation,weapon_asset,imported_weapon,procedural_rifle}.cpp`; the `preview/` executable is
an optional standalone validation target, not part of the game. Ship
`res/shaders/character.vert` and `res/shaders/character.frag`.

1. Own one `silic2::CharacterPresentation`; initialize after the GL context and
   GLAD. Check `init()` and destroy/shutdown before context destruction.
2. Reset at run restart. Call `update(simulationDt, FirstPersonView)` only while
   PLAYING. Do not use wall-clock time for this module. Rendering is time-neutral.
3. Assemble `CharacterView` snapshots for living enemies. `position` is feet;
   `heading` is the actual aim/movement direction. Supply dimensions explicitly:
   Chaser `(0.6,1.8,0.6)`, Sentry `(1.2,1.0,1.2)`. `movement` is actual horizontal
   speed / nominal speed, clamped to 0..1; `phase` is an optional stable offset.
4. Supply Sentry `charging`, `chargeProgress` (whole 1-second charge) and
   `aimLocked` directly from its gameplay state. No attack timer is inferred here.
   Cold ivory `hitFlash` modifies bone independently of warm attack cues.
5. After successful primary `fire`, call `onPrimaryShot()`. Emit `Impact` at the
   actual hit point, `Death` at the dead body's center, and `SentryFire` at its
   muzzle with actual shot direction. Do not emit again from rendering.
6. Use `renderWorld(view, projection, snapshots)` instead of the old enemy boxes.
   Keep existing player/hostile projectile rendering. Draw world effects normally,
   then `renderFirstPerson(projection)` before pixel post-processing and HUD.

The view model uses a fixed 70-degree lens at the supplied aspect ratio to remain
clear during the existing 45/60/75-degree camera FOV transitions. This does not
modify the camera, shot origin, aim, collision, fire cadence, movement or recoil
rules. Small visual recoil, movement bob and slide cant affect only the model.
First-person geometry uses sorted opaque triangles without touching world depth.
Both passes restore the program, VAO, array buffer, depth enable/function/mask,
blend enable and cull enable that they change. No blend function is changed.

World characters and fragments use one streamed draw; the view model uses one.
Effect storage is a fixed 256-shard ring, and patterns do not consume gameplay RNG.
Slides emit short dry dirt flakes only when sliding, moving and grounded.
Deaths scatter pale bone flakes, while attack and hit fragments keep their cue colors.
No full-screen flash, camera shake or gameplay trajectory is introduced.

## Standalone OpenGL preview (macOS)

Run from the repository root with an active desktop session and Homebrew GLFW:

```sh
clang -Iinclude -c src/engine/glad.c -o /tmp/kinetic_glad.o
clang++ -std=c++17 -Wall -Wextra -Iinclude \
  src/character/preview/preview_main.cpp \
  src/character/character_presentation.cpp src/character/weapon_asset.cpp \
  src/character/imported_weapon.cpp src/character/procedural_rifle.cpp src/engine/mesh_asset.cpp src/engine/simple_json.cpp \
  src/engine/shader.cpp src/engine/texture.cpp \
  /tmp/kinetic_glad.o -L/opt/homebrew/lib -lglfw -framework OpenGL \
  -o /tmp/kinetic-preview
/tmp/kinetic-preview
```

It creates a hidden GLFW OpenGL 3.3 context and a 320x200 framebuffer and writes
six `/tmp/kinetic-horror-preview-N.ppm` frames: idle/run, tracking, locked, locked+hit,
death fragments, slide+shot. Convert with `sips -s format png INPUT --out OUTPUT`
on macOS for inspection. The test asserts linked shaders, no GL errors, GL state
restoration, unchanged world depth after first-person rendering, bounded effect
capacity, zero-delta freeze, expiry, and 50 presentation resets.

## Horror pass validation, 2026-10-08

The Chaser is a tall split-face rib cage with actual gaps, a recessed central
body and folded asymmetric limbs. Sentry is a wide calcareous funnel with an
opaque recessed floor; its lip opens from actual charge progress and the locked
state adds a steady amber pupil and three lip marks. These stay amber during
cold hit flashes. All geometry remains inside the authoritative body envelope.
No silhouette appendage extends beyond that envelope at any tested yaw/pose.

CPU-only geometry checks (no context/device use):

```sh
clang -Iinclude -c src/engine/glad.c -o /tmp/kinetic_glad.o
clang++ -std=c++17 -O2 -Wall -Wextra -Iinclude \
  src/character/preview/geometry_check.cpp \
  src/character/character_presentation.cpp src/character/weapon_asset.cpp \
  src/character/imported_weapon.cpp src/character/procedural_rifle.cpp src/engine/mesh_asset.cpp src/engine/simple_json.cpp \
  src/engine/shader.cpp src/engine/texture.cpp \
  /tmp/kinetic_glad.o -o /tmp/kinetic-geometry-check
/tmp/kinetic-geometry-check
```

10,240 combinations of animation, yaw, type and charge/hit state pass finite
geometry, normalized-normal and body-envelope checks. Chaser: 2,700 vertices;
charged Sentry: 1,716. At 18 Chasers + 3 charging Sentries + all 256 fragments,
world geometry totals 62,964 vertices, within the existing 65,536 reserve.
World and first-person draw counts remain one each. Capacity, nonadvancing dt,
expiry and reset checks pass. Debug and Release compilation pass.

Actual GL preview completed in the ATLAS-assigned serialized window on
2026-10-08. Both versions ran at the same 320x200 resolution, 45-degree world
lens, camera and simulation times. All 12 PNGs were inspected. The first pass
revealed an overly regular Sentry lip and yellow lock mark; final geometry uses
uneven lip angles/depth and thicker ribs, and the lock mark is amber. Final CPU
bounds checks and all six GL poses passed after those corrections.

Outputs: `artifacts/acceptance/horror/character/`, including `before-0..5.png`,
`after-0..5.png`, logs and `evidence.json`. Shader links, GL error checks, state
restoration, preserved world depth, bounded effects, frozen zero-dt state,
expiry and 50 presentation resets pass. The preview uses explicit checks even
under NDEBUG; its optional first argument selects the output filename prefix.
GPU usage is finished and was explicitly released to ATLAS. This is evidence
for isolated presentation, not a substitute for final in-scene contrast or
performance regression.

This is a presentation harness, not the milestone's 50 complete application
restarts or combat performance test. In-scene contrast against final lighting,
full run pause/restart integration, simultaneous stress frame timings, Windows/
Linux runtime and the five-player threat-readability gate remain integration and
acceptance checks.

## Sentry shape revision 2 — final GL verified

ATLAS review found that the first horror Sentry still read as a regular wheel at
320x200. The local follow-up offsets the mouth left/up from the shell centroid,
uses three unequal bone plate groups separated by soft seams, varies the outer
silhouette substantially, and adds two unequal hanging tissue folds. Broad muted
brown-pink inner walls separate the mouth from its dark recessed floor. The warm
attack pupil remains at the original body center, inside the eccentric mouth;
small warm creases follow its actual lip. No gameplay or public API changes.

CPU checks: all 10,240 envelope/normal/finite combinations pass. Sentry maximum
is now 1,716 vertices; capped world total is 62,964/65,536. Chaser, view model,
fragment capacity and draw count are unchanged. Debug/Release compilation passes.
`geometry-sentry-revision2.log` records the new CPU result. The final short GL
window passed all six poses and all shader/state/world-depth/effect/reset checks.
All six revised 320x200 PNGs were actually inspected: left-offset mouth, heavier
upper-right bone plate and thin left tissue seam are visible, with a muted inner
wall separated from the black recess. Warm charge/lock remains inside that hole
and separate from simultaneous cold hit feedback. `after-*.png`, `after.log` and
`evidence.json` now describe revision 2. GPU usage has ended and been released to
ATLAS for VECTOR's integration. Final scene/performance and human checks remain
outside this isolated presentation validation.


## Optional imported rifle adapter — 2026-10-17

The initial adapter was validated while disabled with synthetic fixtures. The
real Tripo rifle is now fitted and enabled by tracked
`res/config/weapon_presentation.json`, which points to the local,
ignored model directory `res/models/tripo/orbital_rifle/`, using VECTOR's
`model.import.json` bundle contract. The application reads the config once during
presentation initialization; restarting the run does not reload disk assets.

Import with the offline `tools/assets/import_static_model.py` tool. Normalize the
real model explicitly to +Y up, muzzle -Z, approximately 0.85m length and centered
pivot; choose source rotation after inspecting the exported asset. The fitting
config records positive scale, translation, rotation and the actual muzzle in
normalized asset coordinates. Optional `leftHandGrip` / `rightHandGrip` vectors
locate the two gloves in camera-root coordinates; forearms connect them to lower
frame anchors. The current values are fitted to task `9483a0cd`. Reinitialize
the application after adjusting them. No paths or generation API keys are needed
in character code. Missing, disabled, malformed, over-budget, incomplete or
unloadable assets keep the existing procedural rifle and print a reason.

The CPU adapter accepts at most 20,000 rifle triangles and 16 opaque material
parts. Base-color textures and normals come from the shared bundle loader.
Texture inputs are limited to 4096px per axis and 64MiB total RGBA base levels
(approximately 85.4MiB including mipmaps, plus driver overhead). There is no PBR/normal-map/skeletal
animation support in this presentation adapter. Alpha transparency is unsupported.
Existing glove/finger/forearm geometry adds three tiny cached material batches;
the current grip anchors are fitted to the real rifle, with original glove/finger
shapes retained and forearm segments reaching the lower frame.

Static GPU vertex buffers and material textures are uploaded once. Rendering
uses the unchanged bob, recoil, slide cant and 70-degree first-person lens. The
imported rifle and hands use a private RGBA8/depth24 target at the current scene
viewport size, then one alpha composite onto the scene; world depth is preserved.
Only viewport resize reallocates this target. Shader draws use model matrices;
there is no per-frame imported mesh sort or vertex upload. At 320x200 its nominal
color/depth storage is about 448KB (driver depth storage can vary), plus cached
meshes/textures. Imported geometry adds one draw per material and a composite;
the tiny existing shot flash remains streamed. The flash comes from the explicit
muzzle transformed with the same model pose and retains the existing 35ms timing.
No hitboxes, cadence, damage, rays, projectile origin, camera or enemies change.

Private rendering restores framebuffers, viewport, program, VAO/buffer, active
texture and texture0 binding, unpack alignment, depth state, color mask, clear
values, blend factors/equations, culling and scissor state. GL resources are owned
by `CharacterPresentation` and destroyed before context teardown. No disk/network
operations occur during combat.

CPU-only adapter probe:

```sh
clang++ -std=c++17 -O2 -Wall -Wextra -Iinclude \
  src/character/preview/imported_weapon_check.cpp src/character/weapon_asset.cpp \
  src/engine/mesh_asset.cpp src/engine/simple_json.cpp -o /tmp/weapon-cpu
/tmp/weapon-cpu
```

`preview/imported_weapon_gl_check.cpp` is a separate synthetic-fixture GL harness.
Link with `imported_weapon.cpp`, `weapon_asset.cpp`, `mesh_asset.cpp`,
`simple_json.cpp`, `shader.cpp`, `texture.cpp`, GLAD and GLFW. It checks sampled
texture colors, world depth, touched states, resizing and resource lifecycle.
Its colored triangles are expressly a test fixture, never a generated rifle.
The actual rifle fitting and six action-state previews are recorded below.
Full gameplay performance and human visual acceptance remain integration checks.

Adapter validation on 2026-10-17 (M2 macOS): CPU synthetic-bundle checks passed
for enabled loading, disabled/missing inputs, invalid muzzle/axis, 20,001-triangle
rejection, transactional failure and 100 animated muzzle poses. Six fallback
320x200 frames are byte-identical to HEAD `d7cc173` using the existing character
preview, which also passed GL-state/depth/effect/reset checks. The imported-mesh
harness passed four textured poses, actual sampled colors, private self-occlusion,
unchanged world depth, state restoration, texture failure, viewport resize and ten
init/shutdown cycles. An initial self-occlusion assertion incorrectly expected a
perspectively protruding rear triangle to disappear; the fixture was reduced so
it lies entirely behind the front triangle. A macOS incomplete-sampler warning
was eliminated by binding a complete white texel for untextured materials.

Local evidence: `artifacts/acceptance/tripo/weapon/`. Synthetic preview images
contain colored triangles only. The six fallback frames prove preservation of
existing presentation; neither these nor the synthetic test substitute for the
missing real-rifle fitting, gameplay stress performance or human visual review.


## Actual Tripo rifle fitting — 2026-10-17

ATLAS generated the approved low-resolution-oriented rifle prompt using task
`9483a0cd-145a-4823-9f80-f3d2455a8ae8` (40 credits, reported by ATLAS). KINETIC
made no generation/API calls. Source GLB SHA256:
`98e8ce222b16c16c03096d0aa39079c0389288c6012f4d1408bb7d3c82d499ca`.
Original: `artifacts/tripo/raw/01_orbital_rifle/tripo-out/one-original-rigid-sci-fi-horror-9483a0cd/model.glb`.
Runtime: `res/models/tripo/orbital_rifle/model.import.json`; full producer/task,
prompt hash, source hash and normalization are retained in that manifest.

The actual source had +Y up and its barrel along +Z. This was checked with a
four-angle textured GL turntable and end-slice bounds (narrow circular muzzle at
+Z, broad stock at -Z). Offline Y=180° rotation produces canonical muzzle -Z;
longest dimension is 0.85, AABB center is the origin. Original 5,511 triangles
become 11,022 with the importer’s explicit double-sided reverse faces. One opaque
material uses its original 2048×2048 JPEG, original UVs and normals; no replacement
texture or paid postprocessing was used. The initial review bundle remains under
`artifacts/acceptance/tripo/weapon/review-bundle/` only, not in runtime config.

At 320×200 the first fit placed gloves below the intended foregrip. The final
fit lowers the rifle by .03 camera-root units and sets explicit glove anchors:

- Model fit: translation `[0,-0.04,0.06]`, uniform scale `0.55`, zero rotation.
- Left grip: `[-0.028,-0.045,-0.005]`; right grip: `[0.02,-0.078,0.16]`.
- Asset-local muzzle: `[-0.0008,0.0499,-0.426]`, just outside the observed bore end.

Two short procedural forearms connect the gloves toward the lower frame. The
original fallback gun/hands stay unchanged. Final idle/run and slide+shot images
show a subdued gray/ivory rifle in the lower-right with a clear central aiming
region; the flash starts at the observed muzzle. Normal, charging, locked,
locked+hit, death-fragment and slide+shot frames use the actual asset. The existing
preview asserts unchanged world depth, GL state restoration, bounded/frozen/expired
effects, shader/GL checks and 50 presentation resets, and passed with the shipping
config enabled. CPU loader/fallback/budget/transform tests passed again.

Evidence: `artifacts/acceptance/tripo/weapon/actual-final-0..5.ppm`,
`actual-final-idle.png`, `actual-final-slide-shot.png`, `actual-final-gl.log`,
`cpu-actual-phase.log`, and `actual-evidence.json`. The explicit-config preview
argument permits disabled fallback tests without replacing the live config:
`/tmp/tripo-kinetic-actual-preview OUTPUT_PREFIX CONFIG_PATH`.
`preview/weapon_turntable.cpp` accepts `model.import.json OUTPUT_PREFIX` and
produces four textured views for actual-axis review.

Full in-map stress performance and human grip/weapon-readability judgement remain
ATLAS/VECTOR integration checks. Raw models and textures are local assets excluded
by Git rules; a source-only checkout without them automatically retains the
procedural weapon. This pass does not change gameplay cadence, damage, bullets,
movement, enemies, enemy hitboxes, sound or the first-person lens.

## Handless reflex sight / ADS revision, 2026-11-12

Both the actual imported rifle and procedural fallback now omit every added hand,
forearm, sleeve and finger. The fallback retains its pistol grip; it is a gun part.
The imported fit scale is .66 instead of .55 (20% larger). Fallback gun geometry is
also scaled by 1.2. No source GLB, textures, enemy shapes or gameplay values change.

The optic is an original lightweight hollow twelve-sided reflex hood, inner wall,
mount and adjustment block. It has a clear aperture and a small emissive red disk,
with depth-tested attachment to the rifle. It is geometry in the same private
viewmodel target, not a HUD dot painted over the game. Both real and fallback
meshes use the cached `ImportedWeapon` renderer and its existing private depth
attachment; missing rifle resources still support the full optic presentation.
The sorted procedural drawing path remains only as a final GPU-cache fallback.

`FirstPersonView::aimBlend` is the sole incoming ADS blend (0 hip, 1 sighted).
App/gameplay owns input, smoothing, camera FOV, sensitivity and shot direction.
The viewmodel keeps its fixed 70-degree lens while shifting the optic anchor to
camera x=y=0. Full ADS suppresses lateral bob, slide cant and rotational recoil;
axial recoil remains. Thus the rendered red dot stays on camera forward while
moving, sliding and firing. The world camera and HUD are integrated by VECTOR.
`update(0, snapshot)` synchronizes the snapshot without advancing presentation
clocks, allowing lifecycle state changes; reset clears the incoming state.

Art acceptance executable: `src/character/preview/ads_weapon_check.cpp`, same
sources/libraries as the standalone preview above. It verifies 100 mathematical
ADS poses and captures eight 480x300 frames (actual/fallback hip, half transition,
ADS, moving slide with recoil). The actual rasterized ADS dot occupies four pixels
with centroid exactly (240,150) in both paths, including slide/recoil. World depth
and GL state remain unchanged; init/shutdown, zero-delta state sync and reset pass.
Evidence: `artifacts/acceptance/rifle-ads-art/`. Checker background is a test fixture,
not a game screenshot. Final App input/FOV/HUD acceptance belongs to VECTOR.
No performance tests, asset-generation calls, commits or pushes in this task.

## Current default: Crimson Carapace procedural rifle (2026-11-12)

This section supersedes the imported-rifle default and closed-ring optic above.
`res/config/weapon_presentation.json` now selects `source: procedural`. Default,
missing, malformed and disabled-import configurations resolve to the exact same
new code-generated gun. The legacy loader is reachable only via explicit
`source: imported`; its old fit fields and local resources are still supported.

Build and test seam: add `src/character/procedural_rifle.cpp` to the presentation
sources. `buildProceduralRifle()` returns deterministic CPU geometry without any
file, GL or network operation. `resolveWeaponPresentation(configPath, status)` is
the same source-selection entry used by `CharacterPresentation::init`. The asset
contains no manifest, OBJ, GLB or texture path; every surface uses a code palette.
The existing `ImportedWeapon` name now denotes the generic static-mesh GPU cache,
not a requirement to import a model. No weapon triangles are rebuilt, uploaded or
sorted per frame. Muzzle and world effects retain their existing bounded path.

The approved concept is realized with six separated swept front gills on each
side, interlocking faceted receiver plates, curved multi-bend magazine, angled
grip/trigger guard, split muzzle jaws, sharp stock, two dorsal stock overlays and
beveled left/right cap ridges. Six groups of red energy chambers, zigzag receiver
seams, magazine channels and a connected stock spine have broad red shoulders
and pale red cores. All parts, including the optic, are original generated meshes;
there are no hands, sleeves or fingers. Final static budget is 1,222 triangles,
3,666 expanded GPU vertices, eight material groups and two emission groups. The
32-byte GPU vertex layout uses 117,312 static VBO bytes, excluding existing
private color/depth attachments and the generated 1x1 white sampler texture.

The reflex sight is an open `\_/` yoke with no roof, top crossbar or lens plane.
Its tiny emissive center disk is a separate material excluded from bloom. It
retains the incoming `FirstPersonView::aimBlend` contract and fixed 70-degree
viewmodel lens. Hip position moves down by 0.012 visual units; full ADS anchor,
0.16-second App transition, 32-degree world FOV and actual shot logic are unchanged.
The complete sighted pose suppresses lateral bob/rotation and keeps axial recoil.

Private material/composite protocol: RGBA8 alpha encodes empty=0, opaque≈0.5,
energy source=1. The compositor reconstructs opaque coverage and adds a bounded
three-pixel red glow around energy seams. Reticle alpha stays opaque≈0.5 and never
seeds bloom. The small kernel uses only the viewmodel texture; it changes no world
or HUD target, creates no extra framebuffer and preserves the caller's GL state.
Procedural metal shading adds directional facet/specular highlights and a local
analytic red bounce near energized surfaces. This is stylized emissive metal,
not physically simulated light transport or a global HDR/bloom pipeline.

Art preview: `src/character/preview/procedural_rifle_preview.cpp` captures current
hip, ADS, moving slide/recoil, side, three-quarter and actual `onPrimaryShot`
firing states. Final evidence is `artifacts/acceptance/procedural-rifle/final-art/`.
Gameplay views are 480x300; side and three-quarter inspections are 640x400 on a
checker fixture, not App screenshots. ADS and moving/recoil dot samples remain
four pixels with centroid exactly (240,150). All states preserve world depth and
GL enables/blending/active texture. The current material passes private target
resize 160x100 then 480x300 with caller viewport/FBO/depth restored. Missing-config
public init/shutdown also passes. Final App control/flow checks belong to VECTOR.
No model-service calls, performance benchmarks, commits or pushes were made.

## Compact optic and continuous energy channels (2026-11-12)

The complete existing optic geometry is now scaled to 60% around the established
sight pivot `(0,.095,.08)`, preserving gun pose, size and the ADS eyeline. A narrow
mount adapter joins the reduced assembly back to the original gun contact; the
frame stays physically attached. The original three U segments retain their
material order, with no top bar. The red disk follows the scaled optic's depth
but retains its `.0021` radius for four readable pixels at 480x300. No camera,
shot, ADS transition, world FOV or sensitivity parameter changes.

The two marked energy interruptions were authored geometry gaps: receiver to
stock endpoints were 0.03816m apart, dorsal to rear cap 0.04270m. Main side and
back channels now each use one continuous route with unified Energy/Core widths.
Shared miter vertices seal corners; the back route turns over the rear bevel
with per-point surface normals. The side route stays outside the raised stock
armor ridges and retains normal depth testing. Six separate gill chambers remain
separate. Material shader, bloom kernel/strength and GL depth policy are unchanged.
The final geometry is 1,288 triangles / 3,864 expanded vertices / eight materials,
123,648 static VBO bytes. Armor geometry and body silhouette are preserved.

The fast regression loop renders the actual builder/private FBO at fixed 480x300:

```sh
python3 src/character/preview/check_crimson_refinement.py \
  /tmp/crimson-rifle-preview artifacts/acceptance/crimson-refinement/after-render
```

Compile `procedural_rifle_preview.cpp` with the character sources listed above.
The command checks bright-red 8-connected paths through the two user-marked
regions, excluding dim halo. It failed on the previous renderer (hip component
gap 19px, ADS 63px) and passes with both gaps zero after the geometry fix. The
same original full preview verifies six poses, four-pixel centered ADS/recoil dot,
world-depth/state restoration, 160x100 then 480x300 target resize and missing-config
initialization. Red-point counting uses only the central 8x8 area because the
smaller U frame now legitimately enters the old 20x20 test area. Evidence and
ranked diagnosis are in `artifacts/acceptance/crimson-refinement/`.
