# Crystal Observatory — playable exploration pass

2026-10-13 user direction: change Crystal Arena only, drawing on varied first-person
spaces and Eclipsium's pixel aesthetics; stop restricting the scene to a simple
shooting arena. Reference evidence and access limits: `docs/crystal-world-references.md`.
ATLAS implementation assumption: exploration with one short encounter and an
observation interaction. This is a bounded playable prototype, not a genre change
for Hollow Orbit or a completed full game. Later user steering takes precedence.

## Experience

An abandoned observatory uses enormous crystals to assemble a view of a missing
celestial body. The player follows its fractured image through a compact loop.
Space, light and player attention carry the experience as well as shooting.

1. Light baffle entrance: bent, enclosed approach in dark mineral; a partial view
   of the suspended central crystal through an opening. No immediate enemy wave.
2. Salt-mirror court: a broad, pale gray/mint court with low cover and room to
   circle. One local, finite encounter using existing enemies. Clear it to obtain
   a quiet observation phase. No whole-map 90-second survival clock for this mode.
3. Broken-page gallery: staggered thick crystal screens, alternating long and
   short sightlines, a main path and a legible side route. From movement, fragments
   of a circular marking become recognizable.
4. Calibration window: a safe observation pocket. Standing in the marked area
   and aiming at the common focal point aligns three physically placed fragments.
   Maintain a short steady observation to calibrate; clear world/UI feedback shows
   progress. Position, sightline and facing must drive actual gameplay state.
5. Suspended crystal well and return: the largest landmark responds visibly to
   calibration, with an inward starfield and changed guide markings. Walk to an
   explicit exit to complete the journey, with a view back toward the entrance.

Use a contiguous floor and honest static collisions for this pass. Vertical scale
comes from tall walls, low/high ceilings, suspended structures and distant views.
Keep navigation simple; no required jumping puzzles, real portals, gravity flips,
moving collision, mirrors or volumetric refraction. Explain approximations honestly.
All reachable routes must have corresponding visible surfaces and collision.

## Presentation

Each space changes at least two of silhouette, sightline, brightness, material
scale and player activity. An enlarged square with more pillars fails this goal.
Use dark brown mineral at entry, broad pale salt in the court, ink-blue layered
crystal in the gallery, and ivory edges around the final violet/black core.
Keep one distant, later approachable landmark. Warm threat cues and current enemy
silhouettes remain readable. Design at the current 320x200 scene resolution;
use broad facets and fractures, attenuating fine noise rather than adding sparkle.

Scene anchors, interaction points, routes and materials must derive from authored
map data. Agree shared data interfaces before parallel writes. A small explicit
exploration state controller should own encounter, observation and exit progression;
renderers consume its snapshot and never decide completion.

## Scope and preservation

- Modify the selected `res/maps/starry_crystal_arena.json` experience; keep the
  previous map in the recorded baseline for comparison.
- Preserve default Hollow Orbit, its map/assets and actual fixed-view rendering.
- Preserve all enemy/character/weapon/audio implementations and assets. Reuse
  existing sounds for interaction where appropriate; no new audio production.
- Share existing movement, shooting, damage, pause, result lock and restart rules.
  VECTOR may extract a small value-based Player input entry so GLFW and labelled
  synthetic WASD share the same movement/collision implementation; preserve all
  movement parameters and state rules. No test-mode branches inside Player.
- Keep original `arena_survival` and legacy map behavior. Add an explicitly named
  exploration mode rather than deriving gameplay from visual preset or file path.
- Keep map/editor serialization compatible; an editor roundtrip must retain the
  new settings even if advanced editing uses raw JSON.
- No inventory, skill tree, procedural world generator, new enemies or weapons.

## Ownership and acceptance

VECTOR owns shared data, map JSON authoring, exploration controller, App, build,
tests, necessary HUD integration and editor compatibility. NEBULA owns environment
geometry/renderers, Crystal shaders and scene previews; it supplies geometry and
anchor requirements to VECTOR before map authoring. ATLAS owns references, scope,
file ownership, review and final evidence. Other specialists need separate tasks.

Required checks: meaningful CPU progression/reset/line-of-sight/navigation and
map roundtrip tests; Debug/Release; actual playthrough with labelled synthetic
input through encounter, observation and exit; failure/pause/restart regression;
actual screenshots at every spatial node, at scene resolution and normal window;
Hollow fixed-camera comparison and its existing flow; protected-file hashes.
Measure the new route's frame times under actual GL/audio, including its most
expensive view. Preserve the existing p95<=20ms/p99<=33.3ms/no three consecutive
frames>50ms gate; report sampling length and synthetic behavior, never as human
enjoyment or readability acceptance. Serialize all GPU work.

Complete when a player can enter, understand the route, fight a finite encounter,
perform the observation, see its world consequence and reach the exit, while
the two themes remain isolated. Human exploration/readability assessment remains
pending. No automatic commit or push.
