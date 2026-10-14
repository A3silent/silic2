# silic2 — Technical Architecture

## Shared Map Module and Editor

The game and `tools/map_editor` build the same data-only `Map` and
`SimpleJson`, static OBJ loader and `SpatialWorld` sources. The editor's `SILIC2_ROOT` CMake setting points to this
repository; it no longer compiles a separate format implementation.

`Map` owns parsing, migration, validation, primitive compilation and serialization.
File/JSON loads and typed edits validate a candidate before replacing the current
document. Authored `GeometryObject` boxes/planes and retained legacy meshes
produce one compiled brush collection. Renderers and the existing collision
paths consume those brushes. `ArenaLayout` derives its floor bounds, obstacles
and typed spawn slots from the same document; `ArenaDefinition::fromMap` builds
the navigation graph, which is supplied to enemies instead of a fixed arena.

Explicit game mode selects the director; explicit environment preset selects
scene presentation. `EnvironmentScene` derives floor/rift transforms and timing
without GL, while `EnvironmentRenderer` owns GPU presentation. Anchored lights
resolve through `Map`; authored and resolved lights have separate read views.
The App validates and prepares the candidate renderer/navigation before replacing
its live map. Materials no longer select gameplay.

The editor's `EditorDocument` owns selection, validated edit commands, bounded
undo/redo history and the saved revision. The UI renders a separate logical-size
viewport into the corresponding Retina framebuffer region. The preview is an
authoring view; final game materials, effects and physics still run in the game.

See [format and compatibility](map-format.md) and
[cross-program acceptance](map-editor-upgrade.md). This change preserves the
legacy map behavior; triangle maps opt into the new shared spatial queries.

## Crystal combat world (2026-10-13)

Both shipped arenas select `ArenaSurvival`. Crystal explicitly enables schema v3
`spatial.mode: "triangles"`; Hollow and legacy maps retain their previous paths.
`Map` compiles transformed primitives and static OBJ instances into visual and
collision triangles. `SpatialWorld` hides triangle/BVH intersection, ground
support, upright actor movement and height-aware graph routing. Player movement,
weapons, enemy projectiles and line-of-sight consume the same actual surfaces.
Bounds accelerate queries; they do not fill mesh openings with solid boxes.

Authored graph nodes use actual 3D feet positions. Links are validated for support
and clearance; bridge upper/lower nodes remain separate and connect through ramps.
Shared target routing caches avoid repeating a full graph search for every enemy
on every frame. Existing movement and weapon tuning stays unchanged.

`environment.architecture: "crystal_combat"` selects Crystal's landmark and distant
structures independently of game mode. The 45-second timed transformation changes
presentation only. Reachable structures render the same modeled surfaces used by
collision. Crystal and Hollow retain separate shaders and share postprocessing,
character presentation and audio. The minimap keeps its existing full-map behavior.

`PlayerInput` remains the common value adapter for GLFW and explicitly labelled
synthetic acceptance input. Tests using it traverse actual ramps and passages;
fixed-camera screenshots and forced states are separate presentation probes.
See [plan](../plan/crystal-combat-world.md), [spatial contract](crystal-spatial-contract.md)
and [acceptance](crystal-combat-acceptance.md).

The earlier `ExplorationController` and Observatory fixture remain for backwards
compatibility and regression tests. Neither shipped map now requires observation
or an exit to win. Historical evidence is in [Observatory acceptance](crystal-observatory-acceptance.md).

## Two-minute Sample Integration (2026-10-07)

The accepted sample scope is `plan/two-minute-sample.md`; actual validation lives
in `docs/sample-acceptance.md`. The default entry is `hollow_orbit_arena.json`,
seed 42. An explicit positional map path can select another arena or legacy map.

The 2026-10-08 presentation revision adds Hollow Orbit. The later dual-map decision
keeps its `hollow_orbit` environment separate from the original `starry_crystal`
environment in `starry_crystal_arena.json`. This was the state of the dual-map
pass; the Crystal combat revision above enlarges and adds real vertical routes to Crystal.
Both continue to share the current eldritch enemy presentation.
Current art and validation:
[cosmic horror pass](cosmic-horror-acceptance.md).

- `ArenaDefinition` provides arena geometry constraints and deterministic waypoint
  navigation. `EncounterDirector` advances an event timeline with safe spawn
  retries, 1.2s rift warnings, caps, 90s cancellation and cleanup completion.
- `Enemy` owns Chaser/Sentry behavior. `EnemyAction` is consumed after actor
  iteration; `EnemyProjectileSystem` owns hostile swept wall/player collisions.
- `App` integrates modules and controls READY/PLAYING/PAUSED/WON/DEAD, focus/input
  edges and same-seed resets. Actual HP loss and fatal source feed `RunStats`.
- `HudViewModel` is a value snapshot of state, stats, damage cues and visibility;
  the HUD never owns gameplay transitions. Original glyph geometry is batched and
  rendered after pixel post-processing at the real framebuffer size.
- `EnvironmentRenderer` and `CharacterPresentation` own procedural visual assets.
  World time/state is supplied explicitly, so pause and reset cannot silently
  advance independent visual clocks. The transformation does not mutate geometry.
- `AudioSystem` preloads original PCM and mixes a fixed 24 voices (8 reserved for
  critical cues) through miniaudio 0.11.25. Missing audio has a visible fallback.
- Reset stops audio and transient runtime, resets seed/stats and player/weapon,
  then director/enemies and presentation, and finally unlocks input. GL owners
  are destroyed while the context is current; sound callbacks stop before PCM is freed.

`--acceptance` modes run actual GLFW/OpenGL/audio loops with explicitly marked
synthetic input. They produce JSONL and frame CSV evidence and do not substitute
for human playtests. See the README for launch commands and ownership in
`docs/agent-team.md` for changes to shared modules.

---

## Project Layout

```
silic2/
├── src/
│   ├── engine/     # Application loop, map, rendering, collision
│   ├── game/       # Run state, statistics, deterministic random streams
│   ├── player/     # Player and weapon runtime
│   ├── enemy/      # Enemy behavior and lifecycle
│   ├── effects/    # Particle systems
│   └── hud/        # Native-resolution HUD
├── include/        # Mirrored public headers plus bundled third-party headers
├── res/
│   ├── shaders/    # GLSL shader pairs (.vert / .frag)
│   ├── maps/       # JSON level files
│   └── texture/    # Game textures (target: 64×64 PNG)
├── lib/            # Pre-compiled libraries (glfw3, opengl32)
├── bin/            # Build output (object files)
├── docs/           # Design and technical documentation
├── tests/          # Headless game-layer tests and baselines
├── Makefile.map    # Build system
└── silic2.exe      # Output binary
```

All code lives in the `silic2` namespace.

---

## Source Files

### Core Application

| File | Purpose |
|------|---------|
| `src/engine/main.cpp` | Entry point; parses an optional map path and `--seed <uint64>` |
| `src/engine/app.cpp` / `include/engine/app.h` | Central controller; owns subsystems and connects gameplay events to run state |
| `src/engine/game_config.cpp` / `include/engine/game_config.h` | Singleton config (window / render / player / camera / effects); JSON save/load |

### Rendering

| File | Purpose |
|------|---------|
| `src/pixel_renderer.cpp` / `.h` | Low-res FBO (320×200), GL_NEAREST upscale to window resolution |
| `src/map_renderer.cpp` / `.h` | World geometry renderer; 128-light pipeline; wireframe toggle |
| `src/shader.cpp` / `.h` | OpenGL shader wrapper; uniform setters for all types |
| `src/texture.cpp` / `.h` | STB_IMAGE loader; `TextureManager` singleton with caching |

### Game Systems

| File | Purpose |
|------|---------|
| `src/game/seed_service.cpp` | Stable 64-bit run seed and independent named random streams |
| `src/game/run_stats.cpp` | Run/room timing, combat, damage, and movement measurements |
| `src/game/run_controller.cpp` | Run lifecycle and same-seed restart behavior |
| `src/player/player.cpp` | Physics, AABB collision, movement states, camera effects |
| `src/player/weapon.cpp` | Bullet physics, hit results, dual-pass render, dynamic lighting |
| `src/enemy/enemy_manager.cpp` | Enemy spawning, hit resolution, contact pressure, and cleanup |
| `src/effects/particle_system.cpp` | General and ground particle runtime |

### World / Content

| File | Purpose |
|------|---------|
| `src/engine/map.cpp` / `include/engine/map.h` | Shared map definition, validation, migration, derived layout and serialization |
| `src/engine/simple_json.cpp` / `include/engine/simple_json.h` | Strict JSON parser and serializer shared with the editor |

---

## Run Lifecycle and Determinism

```
map load -> RunController::startRun -> RunStats::beginRoom
PLAYING  -> record movement / shots / hits / kills / actual HP loss
death    -> endRoom(PlayerDied) -> endRun(PlayerDied)
clear    -> endRoom(Cleared)    -> endRun(Completed)
result   -> wait 1 second -> R or click -> reset runtime -> restart same seed
```

`SeedService` derives independent `encounter`, `rewards`, `room_graph`, and
`cosmetic` streams from the run seed using fixed FNV-1a and SplitMix64 mixing.
Simulation decisions must not use `rand()`, `std::random_device`,
`std::hash<string>`, or implementation-defined standard distributions.
Presentation-only randomness remains isolated and must never change encounter
or reward results.

`RunStats` and `RunController` do not depend on GLFW or OpenGL. Run their tests
with `make -f Makefile.map test`.

---

## Rendering Pipeline (per frame)

```
1. PixelRenderer::beginPixelRender()     bind 320×200 FBO
2. MapRenderer::render()                 world geometry + 128-light calculation
3. Weapon::render()                      pass 1: glow halos (additive blend)
                                         pass 2: solid bullet geometry
4. GroundParticleSystem::render()        ground particles
5. PixelRenderer::endPixelRender()       upscale to window via GL_NEAREST
6. glfwSwapBuffers()
```

---

## Shader Files (`res/shaders/`)

| Shader pair | Purpose | Key detail |
|-------------|---------|------------|
| `map.vert/frag` | World geometry | 128-light pipeline; 4-level color palette (2 bits/channel); 4-band brightness stepping |
| `bullet.vert/frag` | Solid bullet geometry | Elongated box, direction-aligned rotation |
| `glow.vert/frag` | Bullet glow halos | Billboard quad; additive blending; smoothstep soft glow |
| `particle.vert/frag` | Point-sprite particles | Per-vertex color, size scaling |
| `particle_box.vert/frag` | 3D box particles | Instanced rendering |
| `pixel.vert/frag` | Post-process upscale | Full-screen quad, GL_NEAREST |
| `simple.vert/frag` | *(unused)* | Legacy |
| `scene.vert/frag` | *(unused)* | Legacy |
| `screen.vert/frag` | *(unused)* | Legacy |

---

## Legacy Map Format (JSON v1.0)

Current v2/v3 fields are documented in [map format](map-format.md).

```json
{
  "version": "1.0",
  "worldSettings": {
    "gravity": -9.8,
    "ambientLight": [r, g, b],
    "backgroundColor": [r, g, b]
  },
  "geometry": {
    "brushes": [{
      "id": 0,
      "material": "floor|wall|platform|...",
      "color": [r, g, b],
      "texture": "res/texture/example.png",
      "vertices": [x,y,z, x,y,z, ...],
      "faces": [i0,i1,i2, ...]
    }]
  },
  "entities": [{
    "type": "player_start|enemy_spawn|item_pickup|trigger",
    "position": [x, y, z],
    "rotation": [x, y, z]
  }],
  "lights": [{
    "type": "point|directional|spot",
    "position": [x, y, z],
    "color": [r, g, b],
    "intensity": 1.0,
    "range": 10.0
  }]
}
```

**Available maps:**

| File | Notes |
|------|-------|
| `hollow_orbit_arena.json` | Default; space-horror survival arena |
| `starry_crystal_arena.json` | Crystal combat world, schema v3 triangle geometry |
| `test_room.json` | Legacy; floor + 4 walls + platform + enemy_spawn + item_pickup |
| `simple_corridor.json` | Long corridor, movement testing |
| `textured_room.json` | Texture support demonstration |
| `kailiang_room.json` | Larger space, multiple lights |
| `complex_base.json` | Most detailed geometry (28 KB) |

---

## Key Subsystem Details

### Player Physics

Player states: `IDLE` → `WALKING` → `RUNNING` → `JUMPING` / `FALLING` → `SLIDING` / `CROUCHING`

Slide trigger: horizontal speed must exceed **5.0 units/s** (`currentSpeed > 5.0f` in `player.cpp`).
Sprint (10.0 units/s) is the normal way to reach that threshold.

Camera height offsets (smoothly interpolated at speed 8.0):
- Normal: 0.0 offset (eye height = 1.6 from config)
- Crouching: −0.6 offset
- Sliding: −0.8 offset

### Weapon System

- Fire rate: 0.2s cooldown (`fireRate = 0.2f`)
- Bullet speed: 65 units/s (hardcoded in `Weapon::fire()`)
- Bullet lifetime: 3.0s
- Bullet mesh: 0.04 × 0.04 × 0.30 units (±0.02 wide, ±0.15 long)
- Glow billboard mesh: 1.0 × 1.0 units, rendered at 0.8 scale
- Bullet collision radius: 0.05 units; 0.1s startup delay (prevents self-collision)
- Impact light lifetime: 0.2s fade-out
- **Bullet lighting is OFF by default** — `bulletLightingEnabled = false` in constructor

### Lighting

- Max lights per frame: 128 (enforced in `map_renderer.cpp` and `map.frag`)
- Attenuation: `max(0, 1 - distance / range)`
- Diffuse: `max(0, dot(normal, lightDir)) * color * intensity * attenuation`
- Static map lights + dynamic bullet/impact lights are merged before upload

### Particle System

| Mode | Rate | Life | Color |
|------|------|------|-------|
| FIRE | 50/s | 2.5s | Orange → Yellow |
| DUST | 25/s | 5.0s | Tan (0.8, 0.7, 0.6) |

Fade LUT cache: pre-computes `lifeRatio^fadeRatio` at 32 smoothness levels per fade ratio — eliminates `std::pow()` per particle per frame.

### Configuration System

`GameConfig::getInstance()` — singleton, sections:
- `WindowConfig` — width (1280), height (720), title, fullscreen
- `RenderConfig` — pixel size (320×200), VSync, near/far planes (0.1 / 100.0)
- `PlayerConfig` — all movement/physics/FOV/slide values
- `CameraConfig` — yaw, pitch, rotation limits
- `EffectsConfig` — particle enable, intensity, emission rate

---

## Development Guidelines

- All code comments and identifiers in **English**
- Use `GameConfig` for all tunable values — no hard-coded gameplay constants
- No raw pointer ownership — `std::unique_ptr` throughout, RAII for all GL resources
- Textures: **64×64 pixels**
- Pixel aesthetic: GL_NEAREST, 4-level palette, 4-band brightness (match `map.frag` style)
- Don't add systems ahead of the phase they're needed in (see `docs/roadmap.md`)
