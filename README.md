# Silic2 - Retro-Style First Person Shooter

A pixel art style first-person shooter game built with OpenGL, featuring Doom-style fast-paced combat and retro 3D visual effects.

## Game Features

### 🎮 Gameplay Experience
- **Retro Pixel Aesthetics** - Low-resolution rendering creates authentic 90s FPS atmosphere
- **Fast-Paced Combat** - Doom-style fluid movement and intense battles
- **Built-in Level Editor** - Map editor with real-time preview and testing (planned)
- **Diverse Weapon System** - Projectile weapons with dynamic lighting effects

### 🔧 Technical Features
- **OpenGL 3.3 Core Profile** - Modern graphics API achieving retro visual effects
- **Pixel-Perfect Rendering** - 320x200 default low-resolution render target (configurable)
- **Particle Effect System** - Advanced particle effects for explosions, smoke, muzzle flash
- **Dynamic Lighting** - Real-time lighting with up to 128 light sources
- **JSON Map Format** - Flexible level data storage and loading

## Build and Run

### Requirements
- **Operating System**: Windows 10/11
- **Compiler**: MinGW-w64 (GCC)
- **Graphics API**: OpenGL 3.3+
- **Build System**: Make

### Quick Start
```bash
# Build the game
make -f Makefile.map

# Run
./silic2.exe

# Load specific map
./silic2.exe res/maps/textured_room.json 
./silic2.exe res/maps/test_room.json
./silic2.exe res/maps/corridor.json

# Quick test commands
make -f Makefile.map test-room      # Run with test room map
make -f Makefile.map corridor       # Run with corridor map
make -f Makefile.map textured-room  # Run with textured room map
```

## Game Controls

### Basic Controls
- **WASD** - Character movement
- **Mouse** - Camera view control
- **Left Mouse Button** - Fire weapon
- **Space** - Jump
- **Shift + W** - Sprint
- **G** - Toggle god mode (free flight + noclip)

### God Mode Controls
- **WASD** - Horizontal movement
- **Space** - Move up
- **Shift** - Move down
- **Mouse** - Look around

### Editor Controls (Planned)
- **F1** - Toggle editor interface
- **Middle Mouse Drag** - Pan view
- **Alt + Mouse** - Rotate camera
- **Ctrl + S** - Save map
- **Ctrl + O** - Open map

## Project Structure
```
silic2/
├── src/                 # Source code
│   ├── main.cpp        # Entry point
│   ├── app.cpp/h       # Main application controller
│   ├── camera.cpp/h    # FPS camera system
│   ├── player.cpp/h    # Player controller with physics
│   ├── weapon.cpp/h    # Weapon and bullet system
│   ├── collision.cpp/h # AABB collision detection
│   ├── map.cpp/h       # Level data management
│   ├── map_renderer.cpp/h    # World geometry renderer
│   ├── pixel_renderer.cpp/h  # Low-res framebuffer system
│   ├── shader.cpp/h    # OpenGL shader wrapper
│   ├── texture.cpp/h   # Texture management
│   ├── game_config.cpp/h     # Configuration system
│   └── simple_json.cpp/h     # JSON parser
├── res/
│   ├── shaders/        # GLSL shader files
│   │   ├── map.vert/frag     # World geometry shaders
│   │   ├── pixel.vert/frag   # Pixel post-processing
│   │   ├── bullet.vert/frag  # Bullet rendering
│   │   └── glow.vert/frag    # Glow effect shaders
│   ├── textures/       # 64x64 pixel textures
│   ├── maps/           # JSON format map files
│   └── screenshots/    # Game screenshots
├── include/            # Header files
├── lib/               # Static libraries (GLFW, GLM, etc)
└── bin/               # Build output
```

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
6. 📋 **Map Editor** - Level design tools (planned)
7. 📋 **Enemy AI** - Intelligent adversaries (planned)
8. 📋 **Audio System** - Sound effects and music (planned)
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