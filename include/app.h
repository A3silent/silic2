#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>

// Forward declaration
struct GLFWwindow;

namespace silic2 {

class MapRenderer;
class Map;
class PixelRenderer;

class App {
public:
    App();
    ~App();

    void run();
    bool loadMap(const std::string& mapFile);

private:
    static constexpr int WINDOW_WIDTH = 1280;
    static constexpr int WINDOW_HEIGHT = 720;
    static constexpr const char* WINDOW_TITLE = "Silic2 FPS Engine";

    GLFWwindow* window;
    std::unique_ptr<Map> currentMap;
    std::unique_ptr<MapRenderer> mapRenderer;
    std::unique_ptr<PixelRenderer> pixelRenderer;

    // Camera
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;  
    glm::vec3 cameraUp;
    float yaw, pitch;
    
    // Timing and input
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    float lastX, lastY;
    bool firstMouse = true;
    
    // Movement settings
    static constexpr float MOVEMENT_SPEED = 5.0f;
    static constexpr float MOUSE_SENSITIVITY = 0.1f;

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