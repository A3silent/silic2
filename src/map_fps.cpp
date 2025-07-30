#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>

#include "map.h"
#include "map_renderer.h"

using namespace silic2;

class MapFPS {
private:
    GLFWwindow* window;
    Map currentMap;
    std::unique_ptr<MapRenderer> mapRenderer;
    
    // Camera
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;  
    glm::vec3 cameraUp;
    float yaw, pitch;
    
    // Timing
    float deltaTime;
    float lastFrame;
    float lastX, lastY;
    bool firstMouse;
    
    const int SCREEN_WIDTH = 1280;
    const int SCREEN_HEIGHT = 720;
    const float MOVEMENT_SPEED = 5.0f;
    const float MOUSE_SENSITIVITY = 0.1f;

public:
    MapFPS() : window(nullptr),
               cameraPos(0.0f, 1.0f, 3.0f),
               cameraFront(0.0f, 0.0f, -1.0f),
               cameraUp(0.0f, 1.0f, 0.0f),
               yaw(-90.0f), pitch(0.0f),
               deltaTime(0.0f), lastFrame(0.0f),
               lastX(SCREEN_WIDTH / 2.0f), lastY(SCREEN_HEIGHT / 2.0f),
               firstMouse(true) {}
                  
    ~MapFPS() {
        cleanup();
    }
    
    bool initWindow() {
        std::cout << "Initializing GLFW..." << std::endl;
        
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        std::cout << "Creating window..." << std::endl;
        
        window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "PixelFPS - Map System", nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        glfwMakeContextCurrent(window);
        
        // Set callbacks
        glfwSetWindowUserPointer(window, this);
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        std::cout << "Window created successfully" << std::endl;
        return true;
    }
    
    bool initOpenGL() {
        std::cout << "Loading OpenGL functions..." << std::endl;
        
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            return false;
        }
        
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
        
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        
        return true;
    }
    
    bool loadMap(const std::string& mapFile) {
        std::cout << "Loading map: " << mapFile << std::endl;
        
        if (!currentMap.loadFromFile(mapFile)) {
            std::cerr << "Failed to load map file: " << mapFile << std::endl;
            return false;
        }
        
        // Initialize map renderer
        mapRenderer = std::make_unique<MapRenderer>();
        if (!mapRenderer->loadMap(currentMap)) {
            std::cerr << "Failed to initialize map renderer" << std::endl;
            return false;
        }
        
        // Set camera to player start position if available
        Entity* playerStart = currentMap.getPlayerStart();
        if (playerStart) {
            cameraPos = playerStart->position;
            std::cout << "Set camera to player start: (" 
                      << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ")" << std::endl;
        }
        
        return true;
    }
    
    void processInput() {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
            
        // Toggle wireframe
        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
            static bool lastF1State = false;
            static bool wireframeMode = false;
            if (!lastF1State) {
                wireframeMode = !wireframeMode;
                if (mapRenderer) {
                    mapRenderer->setWireframeMode(wireframeMode);
                }
                std::cout << "Wireframe mode: " << (wireframeMode ? "ON" : "OFF") << std::endl;
            }
            lastF1State = true;
        } else {
            static bool lastF1State = false;
            lastF1State = false;
        }
        
        // Camera movement
        float velocity = MOVEMENT_SPEED * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += velocity * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= velocity * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
            
        // Vertical movement
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            cameraPos += velocity * cameraUp;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            cameraPos -= velocity * cameraUp;
    }
    
    void processMouseMovement(float xoffset, float yoffset) {
        xoffset *= MOUSE_SENSITIVITY;
        yoffset *= MOUSE_SENSITIVITY;
        
        yaw += xoffset;
        pitch += yoffset;
        
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }
    
    void render() {
        // Set background color based on map settings
        const auto& worldSettings = currentMap.getWorldSettings();
        glClearColor(worldSettings.backgroundColor.x, 
                    worldSettings.backgroundColor.y, 
                    worldSettings.backgroundColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Create matrices
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                               (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 
                                               0.1f, 100.0f);
        
        // Render map
        if (mapRenderer) {
            mapRenderer->render(view, projection);
        }
    }
    
    void run(const std::string& mapFile = "res/maps/test_room.json") {
        if (!initWindow() || !initOpenGL()) {
            return;
        }
        
        if (!loadMap(mapFile)) {
            std::cerr << "Failed to load map, falling back to default scene" << std::endl;
            // Could fall back to hardcoded scene here
            return;
        }
        
        std::cout << "Starting main loop..." << std::endl;
        std::cout << "Controls:" << std::endl;
        std::cout << "  WASD - Move" << std::endl;
        std::cout << "  Mouse - Look around" << std::endl;
        std::cout << "  Space/Shift - Move up/down" << std::endl;
        std::cout << "  F1 - Toggle wireframe" << std::endl;
        std::cout << "  ESC - Exit" << std::endl;
        
        while (!glfwWindowShouldClose(window)) {
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            
            processInput();
            render();
            
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        
        std::cout << "Main loop ended" << std::endl;
    }
    
    void cleanup() {
        if (window) {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }
    
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
        MapFPS* fps = static_cast<MapFPS*>(glfwGetWindowUserPointer(window));
        
        if (fps->firstMouse) {
            fps->lastX = xpos;
            fps->lastY = ypos;
            fps->firstMouse = false;
        }
        
        float xoffset = xpos - fps->lastX;
        float yoffset = fps->lastY - ypos;
        fps->lastX = xpos;
        fps->lastY = ypos;
        
        fps->processMouseMovement(xoffset, yoffset);
    }
};

int main(int argc, char* argv[]) {
    MapFPS fps;
    
    std::string mapFile = "res/maps/test_room.json";
    if (argc > 1) {
        mapFile = argv[1];
    }
    
    std::cout << "PixelFPS - Map System Demo" << std::endl;
    std::cout << "Loading map: " << mapFile << std::endl;
    
    fps.run(mapFile);
    return 0;
}