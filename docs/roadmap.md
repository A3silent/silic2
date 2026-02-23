# silic2 — Development Roadmap

---

## Current Status

Core gameplay loop is complete and playable. You can move, shoot, and kill enemies. Phase 4 is fully debugged and stable. Next priority is Phase 5 (run structure) or Phase 6 UI/HUD work to make the game feel more complete.

---

## Implemented ✅

**Engine & Rendering**
- OpenGL 3.3 Core Profile rendering pipeline
- Pixel-perfect 320×200 → 1280×720 upscaling (GL_NEAREST)
- 4-level color palette + smooth-stepped brightness bands (`map.frag`)
- Up to 128 simultaneous point lights; quadratic attenuation with visible center/edge fade
- Half-Lambert diffuse on all geometry (no fully-black faces)
- TextureManager with caching

**Player System**
- WASD + sprint (Shift toggle) + jump (Space) + crouch + slide (Ctrl) + god mode (G)
- States: IDLE, WALKING, RUNNING, JUMPING, FALLING, SLIDING, CROUCHING
- AABB collision with swept testing; step climbing (≤ 0.3 units)
- Momentum physics: ground friction (25.0), air resistance (0.8), air control (0.4)
- Dynamic FOV: 45° / 60° (sprint) / 75° (slide)
- Head bob and camera shake effects

**Weapon System**
- Bullet firing (LMB), 0.2s fire rate
- Bullet speed: 65 units/s; lifetime: 3.0s
- Swept segment-AABB collision — no tunneling at any framerate
- Bullet aims toward crosshair (converges from offset spawn to screen center at 100 units)
- Dual-pass render: glow halos (additive blend) + solid elongated bullet geometry
- Bullet mesh: 0.04 × 0.04 × 0.30 units; glow billboard: 1.0 × 1.0 (rendered at 0.8 scale)
- Dynamic lighting system exists (off by default — `setBulletLightingEnabled(true)`)
- Impact light flash on hit (0.2s fade)

**Particle System**
- FIRE mode: 50 particles/s, orange→yellow, 2.5s life
- DUST mode: 25 particles/s, tan, 5.0s life
- Fade LUT cache (32 smoothness levels) — eliminates per-particle `pow()` calls

**World & Config**
- JSON map format: brushes, entities (player_start, enemy_spawn, item_pickup, trigger), lights
- `ITEM_PICKUP` entity type defined in `map.h` — data ready, logic missing
- Centralized GameConfig singleton with JSON save/load
- deltaTime capped at 50ms — no physics explosion on slow init frames

---

## Phase 4 — Core Gameplay Loop ✅ COMPLETE

| Task | Status | Notes |
|------|--------|-------|
| **Health / damage system** | ✅ | Player has 100 HP float; `takeDamage(float)`, `isDead()`, `getHp()` |
| **Enemy entity** | ✅ | `Enemy` class: position, HP (3 hits to kill), AABB, gravity, states |
| **Bullet → enemy collision** | ✅ | Swept segment-AABB; 1 damage per bullet hit; no tunneling |
| **Enemy spawn from map** | ✅ | `EnemyManager::spawnFromMap()` reads `enemy_spawn` entities |
| **Room clear detection** | ✅ | `App::roomCleared` flag; prints "Room cleared!" to console |
| **Basic enemy movement** | ✅ | Chases player within 15-unit aggro range at 3 units/s |
| **Contact damage** | ✅ | 20 HP/s per touching enemy (within 1.2-unit horizontal range) |
| **Player respawn** | ✅ | Death triggers once; player respawns at `player_start`, enemies reset |

**Bugs fixed post-implementation:**
- Enemy fell through floor on first frame (deltaTime spike from init — capped at 50ms)
- Enemy stuck in place (zero-thickness floor/platform brushes blocked horizontal movement — now skipped)
- Bullet tunneling at close range (swept segment collision replaces point sphere test)
- Bullet aimed beside target (spawn offset — now aims toward crosshair aimpoint 100 units out)
- "Player died!" spamming every frame (one-time death flag added)

**New files:** `include/enemy.h`, `src/enemy.cpp`, `include/enemy_manager.h`, `src/enemy_manager.cpp`, `res/shaders/enemy.vert/frag`

**Door/portal entities** — deferred to Phase 5 (requires room linking system)

---

## Phase 5 — Run Structure

> Goal: One full roguelite run from start to Boss.

- [ ] Slay the Spire style branching node map UI
- [ ] Act / floor progression system (3 Acts)
- [ ] Augment selection screen — pick 1 of 3 cards
- [ ] Augment effect system and stat modifiers
- [ ] Currency drop from enemies + shop system
- [ ] Rest site / upgrade node logic
- [ ] Boss encounters

---

## Phase 6 — Content & Polish

> Goal: Ship-ready experience.

- [ ] Weapon variety (multiple fire modes: hitscan, bounce, explosive, spread)
- [ ] Advanced particle effects (explosions, muzzle flash, hit effects)
- [ ] UI / HUD (health bar, ammo counter, augment display)
- [ ] Game states: main menu, pause, game over, run complete screen
- [ ] Audio system (gunshots, footsteps, ambient, music)
- [ ] Map editor GUI
- [ ] Procedural room assembly from JSON chunks
