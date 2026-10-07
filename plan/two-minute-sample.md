# Accepted milestone: two-minute starry crystal arena

Status: implementation and automated engineering acceptance completed on
2026-10-07. Human headphone listening and five-person playtests remain pending;
the full milestone is not yet accepted. Evidence: [acceptance record](../docs/sample-acceptance.md).
ATLAS coordinated the six-role team under the user's explicit request
to implement this complete milestone. Earlier phase documents remain
reference material and do not expand this accepted scope.

## Intent and limits

Game motto: 在异变空间中高速移动、主动瞄准，通过短场战斗获得改变打法的武器能力。

This sample tests movement, active aim, readable threats, pacing and atmosphere.
Weapon ability rewards belong to the next milestone. Deliver one arena, the
existing primary weapon, Chaser and Sentry, a 90-second spawn schedule followed
by cleanup, English HUD, positional audio and one audiovisual transformation.
No build rewards, pickups, secondary fire, room graph, boss or metaprogression.
Retain current player movement parameters, shooting and collision; add only the
integration needed for the new systems. Preserve all pre-existing local edits.

Use original procedural geometry, shaders, bitmap glyphs and offline synthesized
audio. Test primarily on the user's M2 MacBook Air. Preserve Windows/Linux build
structure but label their runtime validation pending. No distribution package.

## Space and visual identity

- Open-air arena about 36 x 36 units, flat floor and continuous outer running loop.
- South-side player start looking toward center, one central crystal obstruction
  and two short side covers. Main passages at least 4 units wide; no dead ends,
  mandatory jumping platforms or low tunnels.
- Four fixed cardinal crystal rifts for Chasers and three fixed Sentry locations.
- Dark blue/purple mineral surfaces, cyan living crystals, warm hostile highlights.
  Enemies, projectiles and warnings stay brighter than decoration.
- Procedural world-oriented starry sky with dim nebulae and a distant abnormal
  celestial body; sky has no collision. New geometry uses consistent face winding.
- At 45 seconds, a four-second sky/crystal/lighting/audio transformation settles
  into a new appearance. Geometry, routes, collision and enemy attributes stay
  unchanged. No whiteout, extra forced shake or obscured combat cues.

## Combat baseline

| Object | First-playtest baseline |
| --- | --- |
| Player | 100 HP, existing movement/primary weapon, no automatic healing |
| Chaser | 6 HP, speed 3 units/s, existing 20 HP/s contact damage; chases throughout arena after activation |
| Sentry | 12 HP, stationary, turns toward target; body about 1.2 x 1.0 x 1.2 units |
| Sentry attack | Charge 1s; track for 0.65s then lock aim for 0.35s; recover 2.2s after firing |
| Hostile projectile | Non-homing, speed 10, radius 0.12, damage 15, lifetime 4s; destroyed on wall/player impact |
| Active caps | 18 Chasers, 3 Sentries, 32 hostile projectiles |

Sentries require visibility to start charging and recheck the firing path. Attack
progress drives both visual and audio cues; hit flashes remain distinguishable.
Chasers separate gently; when direct pursuit is blocked, use a validated arena
waypoint graph with deterministic shortest-path ordering. No navmesh/behavior tree.

| Combat time | Add Chasers | Add Sentries |
| --- | ---: | ---: |
| 0s | 2 | 0 |
| 10s | 3 | 0 |
| 22s | 3 | 1 |
| 34s | 4 | 0 |
| 46s | 4 | 1 |
| 58s | 5 | 0 |
| 70s | 5 | 1 |
| 82s | 6 | 0 |
| 90s | Stop generation | Stop generation |

Rifts warn with light and positional sound for 1.2s. Seed establishes candidate
order; select legal Chaser rifts at least 5 units from the player. Retry unsafe or
capped spawns every 0.25 simulation seconds and recheck before materialization.
At 90s cancel unfinished generation/warnings and record reasons. Win requires
time >=90s, no pending generation and no living enemies; clear residual hostile
projectiles on victory. Death ends the run immediately. Target winning duration
90-150s. Default seed 42; preserve --seed and same-seed restart. Actual safety
delays may depend on player behavior; compare identical input in determinism tests.

## Flow, UI and feedback

READY -> PLAYING <-> PAUSED -> WON/DEAD -> same-seed restart.
Enter begins from READY; instruction time is excluded. Esc pauses/resumes and
releases/recaptures the mouse. Freeze combat, warnings, presentation and audio;
reset first mouse sampling on resume. Window focus loss pauses gameplay.
After a 1s result lock, R or a fresh left-click restarts; held fire cannot skip it.

English uppercase HUD: HP, combat time, phase/cleanup message, terrain/rift minimap.
Show enemy dots only while currently in-view and unobstructed. Enemy bars require
visibility and damage within 2s. Include hit/kill cues and damage direction.
Results: WIN/DEAD, time, kills, accuracy, actual HP loss, fatal source, restart hint.
Use original 5x7 bitmap glyphs, batched text, logical-size layout and Retina-aware
framebuffer viewport. Draw UI after scene post-processing.

## Implementation sequence and seams

1. Arena and EncounterDirector: pure definitions/timeline, reset/update/completion
   interface, warning/spawn output, safety retries, cap and completion handling.
   New arena uses director; legacy maps keep static spawns.
2. Sentry and EnemyProjectileSystem: shared per-type dimensions, narrow EnemyAction
   output consumed after iteration, independent hostile projectile runtime with
   swept nearest wall/player checks, hit radius, actual Player::takeDamage results.
   Add separation and arena waypoint navigation, then readable combat cues.
3. HUD and measurement: BitmapFontRenderer and read-only HudViewModel; visibility
   filtering; per-type kills, hostile shots/hits, spawn delays/cancellations and
   fatal source. JSONL includes seed/content version; UI and log use one snapshot.
4. Audio and art: procedural sky/crystals and transformation; miniaudio 0.11.25,
   one implementation translation unit, vendored source/license/checksum using
   MIT No Attribution option. Fixed 24 voices, 8 prioritized for critical cues.
   Original preloaded 48kHz/16-bit mono WAV and generators; no runtime Python or
   combat-time file I/O. Stereo positioning/distance attenuation, no HRTF promise.
   Mute fallback is visible and does not pass the audio playtest requirement.
5. Integrate and measure: stop old runtime/audio, reset seed/stats, reset player and
   weapon, initialize director/enemies, reset presentation, then unlock input.
   Destroy GL resources before their context and sounds before their backend.
   Arena becomes default launch; preserve explicit legacy map loading. Update docs.

## Engineering acceptance

- Existing Debug/Release tests pass. New tests cover director events at 30/60/144
  FPS and irregular steps, reproducibility, caps/safety, pause/restart, deadline
  cancellation, completion, Sentry states/visibility and projectile nearest-hit.
- Verify actual-damage/fatal-source stats, map safety/connectivity and readable UI
  at 1280x720, 800x600, Retina and resize. Keep legacy maps functioning.
- Perform 50 full runtime restarts; inspect entity/projectile/warning/audio/stat
  cleanup and absence of sustained post-warmup memory growth. Controller-only
  reset tests do not substitute for this integration check.
- Headphone checks cover directions, simultaneous cues, death and pause.
- M2 Mac Release, 1280x720 logical window, 320x200 internal framebuffer; record
  actual Retina framebuffer. Default VSync on; support off for measurement.
- Measure uncapped wall-clock frame time excluding loading/pause/refocus. Three
  complete runs: p95 <=20ms, p99 <=33.3ms, no three consecutive frames >50ms.
  Stress 18 Chasers +3 Sentries +32 enemy projectiles +2000 environment particles.
  Optimize measured bottlenecks rather than replacing the renderer speculatively.

## Five-person acceptance gate

User organizes five ordinary keyboard/mouse FPS players unfamiliar with the code,
on the same Mac/headphones/default seed. Each plays two rounds before independently
choosing a third. Give only in-game controls, not the enemies' optimal answers.
Record run data, observed behavior, player wording and designer interpretation.

- At least 4/5 correctly explain the main injury/death source.
- At least 3/5 improve on round two.
- At least 3/5 voluntarily start round three.
- No blocking stuck state, incorrect result or leaked restart state.

Improvement: failed players reach later scheduled progress or first win; players
winning twice reduce damage >=10%, or cleanup time >=10% without increased damage.
Use behavior evidence; survival time alone is insufficient.
If explanations fail, iterate cues; if learning fails, iterate waves/routes/difficulty;
if replay desire fails, inspect feedback and downtime. Change one main variable
group at a time and version results. Do not add rewards to bypass this gate.
Engineering completion without these observations is "playtest pending", not full
milestone completion. Deliver code/assets/generators/licenses/tests/performance
records/start instructions and authentic playtest results.
