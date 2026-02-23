# silic2 — Game Design Document

---

## 1. Identity & Vision

**silic2** is a fast-paced, 3D retro-style roguelite shooter combining:

| Inspiration | What it contributes |
|-------------|-------------------|
| **Doom** | Visceral first-person combat, retro aesthetics |
| **Slay the Spire** | Strategic branching run routing between encounters |
| **Soul Knight** | Chaotic, highly randomized weapon variety |
| **Titanfall 2 / Apex Legends** | Fluid, momentum-based movement as a core skill |
| **Vampire Survivors** | Stackable synergistic augments that define each run |

**Core philosophy: Power Fantasy and Fluidity ("爽为主")**
The game rewards mechanical skill and aggressive movement. Every run is unique through weapon RNG and augment synergies. Difficulty should feel satisfying, never punishing.

---

## 2. Core Movement & Controls

The physics engine must guarantee zero friction with environment geometry — the player should **never get stuck**.

| Input | Action |
|-------|--------|
| WASD | Movement |
| Shift | Sprint (toggle) |
| Ctrl | Crouch / Slide (while sprinting, speed > 5.0) |
| Space | Jump |
| LMB | Fire |
| E | Interact |
| G | God mode (dev tool) |

### Movement Principles

- **Momentum-Based Sliding:** Sprint → Ctrl = slide with full momentum carry. Lets players dodge projectiles while maintaining speed.
- **Frictionless Environment:** Auto-step over small obstacles (≤ 0.3 units). Wall-slide without snagging. No sticky geometry.
- **Air Control:** Responsive mid-air strafing (0.4 coefficient) for aggressive repositioning.
- **FOV Feedback:** 45° (normal) → 60° (sprint) → 75° (slide). Already implemented.

### Current Physics Values (from `game_config.h`)

| Parameter | Value |
|-----------|-------|
| Walk speed | 5.0 units/s |
| Sprint speed | 10.0 units/s |
| Slide speed (initial) | 15.0 units/s |
| Crouch speed | 2.0 units/s |
| God mode speed | 15.0 units/s |
| Jump velocity | 8.0 units/s |
| Gravity | −20.0 m/s² |
| Max fall speed | −50.0 units/s |
| Slide friction | 4.0 |
| Max slide duration | 2.0 s |
| Ground friction | 25.0 |
| Air resistance | 0.8 |
| Air control | 0.4 |

---

## 3. Game Loop & Structure

A complete run clears **3 Main Acts**.

### 3.1 Map Routing (Slay the Spire Style)

Before each floor, the player sees a branching node map and chooses their path.

| Node Type | Description |
|-----------|-------------|
| **Normal Combat** | Standard enemy wave; doors lock until all enemies die |
| **Elite / Miniboss** | Harder fight, guaranteed high-tier augment or weapon |
| **Vault / Treasure Room** | Risk-free; contains powerful randomized weapon |
| **Shop** | Spend enemy-dropped currency on HP, weapons, or augments |
| **Rest Site** | Heal HP or upgrade an existing weapon/augment |
| **Boss** | Act finale — defeat to advance to the next Act |

### 3.2 Room Generation (Binding of Isaac Style)

- Floors are assembled from interconnected rooms built from predefined JSON map chunks.
- Combat room doors **lock on entry** and only unlock when all enemies are defeated.

---

## 4. Combat & Progression

### 4.1 Weapons (Soul Knight Style)

- Players start with a basic weapon; new weapons are found in Vault Rooms or purchased in Shops.
- Arsenal ranges from hitscan shotguns to chaotic projectile launchers — bouncing lasers, explosive plasma, shotgun-snipers.
- Projectiles act as dynamic light sources (engine system exists, disabled by default — enable via `setBulletLightingEnabled(true)`).

### 4.2 Augments (Vampire Survivors Style)

After clearing specific nodes or on level-up, the player picks **1 of 3** random Augment cards. Augments stack and synergize heavily.

| Augment | Effect |
|---------|--------|
| *Ricochet* | Bullets bounce off walls once |
| *Vampirism* | Critical hits heal the player |
| *Momentum Damage* | Damage scales with current movement speed — rewards sliding/sprinting |
| *Explosive Rounds* | Enemies explode on death, dealing AoE damage |

### 4.3 The Protagonist

No hero classes or innate abilities. All power scaling comes from player mechanical skill, weapon RNG, and augment synergies. **Blank slate by design.**

---

## 5. Visuals & Aesthetics

- **Retro 3D:** OpenGL rendering upscaled from a 320×200 framebuffer via GL_NEAREST. Already implemented.
- **Pixel-Perfect:** Nearest-neighbor texture filtering. 4-level color palette (2 bits per channel, simulating EGA/VGA). 4-band brightness stepping (bright / normal / dark / very dark) in `map.frag`.
- **VFX:** Ground particle system (FIRE and DUST modes), glowing bullet halos (always on), impact flash lights. All in engine.
- **All textures** should be 64×64 pixels.
