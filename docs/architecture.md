# silic2 — Technical Architecture

---

## Project Layout

```
silic2/
├── src/            # All .cpp source files
├── include/        # All .h headers (+ third-party: glad, GLFW, glm, stb, KHR)
├── res/
│   ├── shaders/    # GLSL shader pairs (.vert / .frag)
│   ├── maps/       # JSON level files
│   └── texture/    # Game textures (target: 64×64 PNG)
├── lib/            # Pre-compiled libraries (glfw3, opengl32)
├── bin/            # Build output (object files)
├── docs/           # Design and technical documentation
├── Makefile.map    # Build system
└── silic2.exe      # Output binary
```

All code lives in the `silic2` namespace.

---

## Source Files

### Core Application

| File | Purpose |
|------|---------|
| `src/main.cpp` | Entry point; accepts optional map path argument, falls back to `res/maps/test_room.json` |
| `src/app.cpp` / `include/app.h` | Central controller; owns all subsystems via `unique_ptr`, runs the game loop |
| `src/game_config.cpp` / `include/game_config.h` | Singleton config (window / render / player / camera / effects); JSON save/load |

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
| `src/player.cpp` / `include/player.h` | Physics, AABB collision, all movement states, camera effects |
| `src/camera.cpp` / `include/camera.h` | FPS mouse-look; Euler angles; view/projection matrices |
| `src/collision.cpp` / `include/collision.h` | AABB vs AABB, swept AABB, ray-AABB, penetration resolution |
| `src/weapon.cpp` / `include/weapon.h` | Bullet physics, dual-pass render, dynamic lighting system |
| `src/particle_system.cpp` / `include/particle_system.h` | General particle system with LUT-optimized fade (32 levels) |
| `src/groundparticle.cpp` / `include/groundparticle.h` | Ground particle factory; FIRE and DUST modes |

### World / Content

| File | Purpose |
|------|---------|
| `src/map.cpp` / `include/map.h` | Level container: brushes, entities, lights; JSON serialization |
| `src/simple_json.cpp` / `include/simple_json.h` | Custom JSON parser (null/bool/number/string/array/object) |

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

## Map Format (JSON v1.0)

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
| `test_room.json` | Default; floor + 4 walls + platform + enemy_spawn + item_pickup |
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
