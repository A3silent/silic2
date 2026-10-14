# KINETIC character presentation

Original procedural eldritch bone/tissue creatures and worn mechanical view model
for the accepted sample, revised by `plan/cosmic-horror-art-pass.md`. No downloaded
assets, textures, runtime asset generation scripts or additional abilities.

## Integration

Build `src/character/character_presentation.cpp` only; the `preview/` executable is
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
  src/character/character_presentation.cpp src/engine/shader.cpp \
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
  src/character/character_presentation.cpp src/engine/shader.cpp \
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
