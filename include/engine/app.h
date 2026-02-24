#pragma once

#include <memory>
#include <string>
#include "engine/camera.h"
#include "engine/run_state.h"

// Forward declaration
struct GLFWwindow;

namespace silic2 {

class MapRenderer;
class Map;
class PixelRenderer;
class Player;
class Weapon;
class GroundParticleSystem;
class EnemyManager;
class Crosshair;
class Minimap;
class HudRenderer;

class App {
public:
    App();
    ~App();

    void run();
    bool loadMap(const std::string& mapFile);

private:
    GLFWwindow* window;
    std::unique_ptr<Map> currentMap;
    std::unique_ptr<MapRenderer> mapRenderer;
    std::unique_ptr<PixelRenderer> pixelRenderer;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Player> player;
    std::unique_ptr<Weapon> weapon;
    std::unique_ptr<GroundParticleSystem> groundParticles;
    std::unique_ptr<EnemyManager> enemyManager;
    std::unique_ptr<Crosshair>    crosshair;
    std::unique_ptr<Minimap>      minimap;
    std::unique_ptr<HudRenderer>  hudRenderer;

    // Game state (replaces bool roomCleared / bool playerDead)
    GameState gameState  = GameState::PLAYING;
    RunState  runState;
    float     stateTimer = 0.0f;

    // Input edge-detection
    bool escWasPressed = false;

    // Timing
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    bool initWindow();
    bool initOpenGL();
    void processInput();
    void update(float deltaTime);
    void render();
    void cleanup();

    // State helpers
    void setState(GameState next);
    void updatePlaying(float dt);
    void handleRoomCleared(float dt);
    void handlePlayerDead(float dt);

    // Callbacks
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};

} // namespace silic2