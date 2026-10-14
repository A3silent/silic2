# Silic2 - Retro-Style First Person Shooter

A pixel art style first-person shooter game built with OpenGL, featuring Doom-style fast-paced combat and retro 3D visual effects.

## Current Sample: Hollow Orbit

The default map is a 36×36 space-horror arena inside a decaying orbital structure.
Calcified growths, an eclipsed celestial mass and hollow creatures establish its
dark atmosphere. Move fast, aim manually, survive
90 seconds of scheduled Chaser/Sentry arrivals, then clear the survivors. At 45
seconds the sky and ossified structures transform over four seconds. The sample includes one
primary weapon, first-person presentation, directional sound, an English bitmap HUD,
pause/results and same-seed restart. Weapon abilities and progression are later work.
The Crystal map is now a larger **Crystal Combat World**: five connected regions
inside a crystal-invaded orbital megastructure, with curved upper walks, wide ramps,
a real bridge underpass, leaning supports and a crystal vault. It uses the same
90-second arrivals and cleanup victory as Hollow Orbit. The previous observation
puzzle is no longer part of this map. Both maps share the current enemies, weapon
and audio. See the [confirmed plan](plan/crystal-combat-world.md),
[acceptance evidence](docs/crystal-combat-acceptance.md),
[spatial authoring contract](docs/crystal-spatial-contract.md) and
[reference study](docs/crystal-world-references.md).
Four original 64×64 material textures now add metal, mineralized ceramic,
weathered shell and crystal detail; see the [texture pass](docs/crystal-texture-pass.md).

Launch from the repository root so shader, map and sound paths resolve. Default seed
is **42**, default VSync is on, and rendering uses a 320×200 scene with a native
resolution HUD. This is a sample under acceptance, not a released game; actual
checks and outstanding human playtests are tracked in
[sample acceptance](docs/sample-acceptance.md) and the current
[horror presentation pass](docs/cosmic-horror-acceptance.md). See [中文说明](README_CN.md),
[accepted scope](plan/two-minute-sample.md) and [agent team](docs/agent-team.md).

## Game Features

### 🎮 Gameplay Experience
- **Retro Pixel Aesthetics** - Low-resolution rendering creates authentic 90s FPS atmosphere
- **Fast-Paced Combat** - Doom-style fluid movement and intense battles
- **Companion Map Editor** - `tools/map_editor` shares the game's map data module; see [upgrade details](docs/map-editor-upgrade.md).
- **Primary Weapon** - Projectile shooting with dynamic lighting and a worn mechanical view model

### 🔧 Technical Features
- **OpenGL 3.3 Core Profile** - Modern graphics API achieving retro visual effects
- **Pixel-Perfect Rendering** - 320x200 default low-resolution render target (configurable)
- **Particle Effect System** - Advanced particle effects for explosions, smoke, muzzle flash
- **Dynamic Lighting** - Real-time lighting with up to 128 light sources
- **JSON Map Format** - Flexible level data storage and loading

## GitHub asset policy

The remote contains source, map JSON, shaders, generators, manifests, prompts and
text validation records. Models, material textures, audio, screenshots and videos
stay local, as do build outputs. Third-party build dependencies remain included.
Historical evidence images and asset hashes describe the full local version; media
links will be absent in a source-only clone. Existing remote history is not rewritten.

Before running or testing a fresh clone, regenerate the required Crystal geometry
and audio from the repository root:

```bash
python3 tools/environment/generate_crystal_combat.py
python3 tools/audio/generate.py
```

The four AI textures require a separate local asset backup in
`res/texture/crystal_combat/`; the Crystal renderer falls back to procedural
materials when they are missing. Restore textures referenced by other old maps
separately. Asset extensions are ignored; do not force-add them to future commits.

## Build and Run

### Requirements
- **Operating System**: Windows 10/11, macOS, or Linux
- **Compiler**: MinGW-w64 (Windows), Apple Clang (macOS), or GCC (Linux)
- **Graphics API**: OpenGL 3.3+
- **Build System**: Make
- **GLFW**: Bundled static library on Windows; install with `brew install glfw` on macOS

### Quick Start
```bash
# Build the debug version (use -j to compile independent files in parallel)
make -f Makefile.map -j8

# Run (Windows)
./silic2.exe

# Run (macOS/Linux)
./silic2

# Reproduce a run with a fixed 64-bit seed
./silic2 --seed 42
./silic2 res/maps/test_room.json --seed 42

# Crystal Combat World: connected regions, ramps and two walkable levels
./silic2 res/maps/starry_crystal_arena.json --seed 42

# Space-horror environment (default)
./silic2 res/maps/hollow_orbit_arena.json --seed 42

# Load specific map
./silic2.exe res/maps/textured_room.json 
./silic2.exe res/maps/test_room.json
./silic2.exe res/maps/simple_corridor.json

# Quick test commands
make -f Makefile.map test-room      # Run with test room map
make -f Makefile.map corridor       # Run with corridor map
make -f Makefile.map textured-room  # Run with textured room map

# Run headless gameplay-system tests
make -f Makefile.map test
```

The Makefile detects the host platform and selects the appropriate compiler,
OpenGL libraries, and executable name automatically.

### Build Configurations

```bash
# Explicit debug build: -O0, full debug symbols
make -f Makefile.map -j8 debug

# Release build: -O2, NDEBUG; outputs silic2-release(.exe)
make -f Makefile.map -j8 release

# Build and run the selected release configuration
make -f Makefile.map run-release

# Remove only the selected configuration
make -f Makefile.map BUILD=debug clean

# Optional compiler cache (requires ccache or sccache to be installed)
make -f Makefile.map -j8 CACHE=ccache
```

Debug and release objects are kept under separate platform-specific directories,
for example `bin/macos/debug/obj`. Generated dependency files ensure that changing
a header rebuilds only the source files that include it.

## Game Controls

### Basic Controls
- **Enter** - Begin from READY; instructions do not consume combat time
- **WASD** - Character movement
- **Mouse** - Camera view control
- **Left Mouse Button** - Fire weapon
- **Space** - Jump
- **Shift** - Toggle sprint
- **Ctrl** - Crouch / slide
- **Escape** - Pause/resume
- **R or Left Mouse Button** - Restart one second after death or room clear
- **G** - Toggle god mode (free flight + noclip)

### God Mode Controls
- **WASD** - Horizontal movement
- **Space** - Move up
- **Shift** - Move down
- **Mouse** - Look around

### Map Editor

Build and launch the editor from the repository root:

```bash
cmake -S tools/map_editor -B tools/map_editor/build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build tools/map_editor/build -j4
./tools/map_editor/build/map_editor res/maps/starry_crystal_arena.json
```

The editor supports primitive ramps and static OBJ instances, transforms/snapping,
duplication, collision/navigation layers and undo/redo. It shares the game loader
and spatial module. Save As rebases model references but does not copy assets.
See its [controls and build instructions](tools/map_editor/README.md) and the
[v3 spatial contract](docs/crystal-spatial-contract.md).
It shares the game's [map format and loader](docs/map-format.md).

## Project Structure

The source tree is organized under `src/engine`, `src/game`, `src/player`,
`src/enemy`, `src/hud`, `src/environment`, `src/character`, `src/audio` and
`src/effects`, with mirrored headers under `include`. Maps, shaders and sounds
live in `res`; configuration-specific build output lives in `bin`.

`tools/audio/generate.py` reproduces the original WAVs; Python is needed only for
regenerating assets. miniaudio 0.11.25 is vendored with its
[license and provenance](third_party/miniaudio/PROVENANCE.md).

## Reproducible Engineering Checks

```bash
make -f Makefile.map BUILD=debug test
make -f Makefile.map BUILD=release test
make -f Makefile.map BUILD=release -j8
./silic2-release --help
./silic2-release --acceptance screenshots --output /tmp/silic2-screens
./silic2-release --acceptance flow --output /tmp/silic2-flow
./silic2-release --acceptance restarts --output /tmp/silic2-restarts
./silic2-release --acceptance performance --vsync off --output /tmp/silic2-perf
./silic2-release --acceptance stress --vsync off --output /tmp/silic2-stress
```

Acceptance modes explicitly use synthetic input and do not establish human playability.
Keep the window focused and avoid other GPU work during performance measurements.
JSONL records and frame CSVs provide evidence; a successful compile is not a runtime
acceptance result. Windows/Linux runtime verification remains separate from macOS.

## Technical Architecture

### Rendering Pipeline
- **Low-Resolution Target** - Pixel-perfect retro visual effects
- **Quantized Lighting** - 8-level light quantization simulating early 3D games
- **Multi-Pass Rendering** - Separate passes for world, bullets, and effects
- **Dynamic Light System** - Bullets act as moving light sources

### Core Systems
- **Configuration-Driven** - Centralized GameConfig with JSON persistence
- **Resource Management** - Texture caching with singleton pattern
- **Smart Pointers** - RAII and automatic memory management
- **Collision System** - AABB physics with swept collision detection

### Development Progress
1. ✅ **Core Engine** - Rendering, input, camera systems
2. ✅ **Basic Game Systems** - Player control, physics, collision
3. ✅ **Weapon System** - Bullet shooting with dynamic lighting
4. ✅ **Configuration System** - JSON-based settings management
5. 🔄 **Particle Effects** - Advanced visual effects (in progress)
6. ✅ **Map Editor** - Shared map data, undo/redo, viewport selection and safe saving; see [validation](docs/map-editor-upgrade.md)
7. 🔄 **Arena Enemies** - Chaser navigation and telegraphed Sentry projectiles (sample acceptance)
8. 🔄 **Audio System** - Original synthesized sounds and spatial mixing (sample acceptance)
9. 📋 **Game Content** - Additional weapons, levels (planned)

## Recent Features

### Weapon System
- **Bullet Physics** - Realistic projectile motion with gravity
- **Visual Effects** - Elongated bullets with glowing halos
- **Dynamic Lighting** - Each bullet emits light affecting the scene
- **Impact Effects** - Lingering light at impact points (0.2s fade)
- **Configurable** - Toggle bullet lighting while keeping visual effects

### God Mode
- Press **G** to toggle god mode for development
- Free flight movement with no collision
- Useful for testing and exploring maps

## Contributing

Issues and Pull Requests are welcome! Please ensure:
- Follow existing code style
- Add appropriate comments in English
- Test new features and fixes

## Screenshots

Screenshots are available in `res/screenshots/` directory for reference.

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file for details.

## Acknowledgments

- **GLFW** - Window and input management
- **GLM** - OpenGL mathematics library
- **stb_image** - Image loading
- **GLAD** - OpenGL function loader
- **Claude Code** - Development assistance
