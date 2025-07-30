# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview
This is a 3D FPS game project called PixelFPS - a Doom-like game with pixel art aesthetics featuring:
- OpenGL-based 3D rendering with low-resolution pixel style
- Advanced particle effects system
- Custom map editor with real-time preview
- Single-player campaign with fast-paced combat

## Technology Stack
- **Graphics API**: OpenGL 4.3+
- **Language**: C++ (engine) + Python (tools/scripting)
- **Libraries**: GLFW, GLM, stb_image, Dear ImGui
- **Audio**: OpenAL or FMOD
- **Build System**: CMake with vcpkg/Conan

## Development Commands

### Build Commands
```bash
# Configure with CMake
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build the project
cmake --build build

# Run tests (when implemented)
ctest --test-dir build
```

### Running the Application
```bash
# Run the game
./build/PixelFPS

# Run with editor mode
./build/PixelFPS --editor

# Load specific map
./build/PixelFPS --map assets/maps/level1.json
```

## Architecture Overview

### Core Engine Structure
The engine follows a modular architecture with clear separation of concerns:

1. **Renderer System** (`src/engine/renderer/`)
   - Low-resolution render targets (320x200 or 640x400)
   - Pixel-perfect rendering with quantized lighting
   - Shader management for vertex, fragment, compute, and post-processing

2. **Particle System** (`src/engine/particles/`)
   - GPU-based particle simulation using compute shaders
   - Supports various effects: explosions, smoke, muzzle flash, magic effects
   - Instance rendering for performance

3. **Game Systems** (`src/game/`)
   - Player controller with FPS mechanics
   - Weapon system (melee, firearms, special weapons)
   - Enemy AI with state machine (IDLE, PATROL, ALERT, ATTACK, DEAD)

4. **Map Editor** (`src/editor/`)
   - Dear ImGui-based interface
   - 3D viewport with fly camera
   - Real-time testing capability
   - JSON-based map format

### Key Design Patterns
- **Entity-Component System**: For game objects
- **Object Pooling**: For particles, bullets, and temporary effects
- **Smart Pointers**: Automatic memory management
- **Observer Pattern**: For event handling

## Performance Considerations
- Use batch rendering for objects with same materials
- Implement frustum culling for visibility optimization
- Texture atlasing to reduce state changes
- GPU compute shaders for particle updates

## Map Data Format
Maps are stored as JSON with the following structure:
- World settings (gravity, ambient light)
- Geometry brushes (vertices, faces, materials)
- Entity placements
- Light definitions

## Development Phases
Currently planning 6 phases:
1. Core Engine (rendering, input, camera)
2. Basic Game Systems (player, physics, audio)
3. Map Editor implementation
4. Particle Effects system
5. Game Content (weapons, AI, levels)
6. Optimization and Polish

## Important Notes
- Maintain pixel-perfect UI rendering
- Use quantized color levels for lighting (8-level quantization)
- Keep low-resolution aesthetic while using modern techniques
- Editor should support undo/redo functionality
- All textures should be 64x64 pixels for consistency