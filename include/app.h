#pragma once

#include <memory>
#include <string>
#include "camera.h"

// Forward declaration
struct GLFWwindow;

namespace silic2 {

class MapRenderer;
class Map;
class PixelRenderer;
class Player;

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
    
    // Timing
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    bool initWindow();
    bool initOpenGL();
    void processInput();
    void update(float deltaTime);
    void render();
    void cleanup();

    // Callbacks
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};

} // namespace silic2