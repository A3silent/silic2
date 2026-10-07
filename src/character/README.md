# KINETIC character presentation

Original procedural meshes and shaders for the accepted sample. No downloaded
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
   Cold white/cyan `hitFlash` modifies armor independently of warm attack cues.
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
Slides emit short cyan ground chips only when sliding, moving and grounded.
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
six `/tmp/kinetic-preview-N.ppm` frames: idle/run, tracking, locked, locked+hit,
death fragments, slide+shot. Convert with `sips -s format png INPUT --out OUTPUT`
on macOS for inspection. The test asserts linked shaders, no GL errors, GL state
restoration, unchanged world depth after first-person rendering, bounded effect
capacity, zero-delta freeze, expiry, and 50 presentation resets.

Validated on the current Mac on 2026-10-07: the above checks pass. Visual inspection
at the sample's 45-degree camera FOV confirms distinct tall Chaser/low Sentry
silhouettes, amber locked-charge aperture and cold hit armor. Initial view-model
clipping and crystal-axis issues were found in preview and corrected.

This is a presentation harness, not the milestone's 50 complete application
restarts or combat performance test. In-scene contrast against final lighting,
full run pause/restart integration, simultaneous stress frame timings, Windows/
Linux runtime and the five-player threat-readability gate remain integration and
acceptance checks.
