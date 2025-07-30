#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "app.h"
#include "map.h"
#include "map_renderer.h"
#include "pixel_renderer.h"
#include <iostream>
#include <stdexcept>

namespace silic2 {

App::App() : window(nullptr),
             cameraPos(0.0f, 5.0f, 5.0f),
             cameraFront(0.0f, -0.5f, -0.5f),
             cameraUp(0.0f, 1.0f, 0.0f),
             yaw(-90.0f), pitch(-30.0f),
             lastX(WINDOW_WIDTH / 2.0f), lastY(WINDOW_HEIGHT / 2.0f) {
             
    if (!initWindow()) {
        throw std::runtime_error("Failed to initialize window");
    }
    if (!initOpenGL()) {
        throw std::runtime_error("Failed to initialize OpenGL");
    }
    
    // Initialize after OpenGL is ready
    try {
        std::cout << "Creating Map..." << std::endl;
        currentMap = std::make_unique<Map>();
        std::cout << "Map created successfully" << std::endl;
        
        std::cout << "Creating MapRenderer..." << std::endl;
        mapRenderer = std::make_unique<MapRenderer>();
        std::cout << "MapRenderer created successfully" << std::endl;
        
        std::cout << "Creating PixelRenderer..." << std::endl;
        pixelRenderer = std::make_unique<PixelRenderer>();
        // Initialize with 320x200 resolution for authentic retro look
        if (!pixelRenderer->init(320, 200)) {
            throw std::runtime_error("Failed to initialize PixelRenderer");
        }
        std::cout << "PixelRenderer created successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize rendering system: " << e.what() << std::endl;
        throw;
    }
    
    // Set callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    
    // Capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

App::~App() {
    cleanup();
}

void App::cleanup() {
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

bool App::initWindow() {
    std::cout << "Initializing GLFW..." << std::endl;
    
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    std::cout << "Creating window..." << std::endl;
    
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Silic2 FPS Engine", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window);
    std::cout << "Window created successfully" << std::endl;
    return true;
}

bool App::initOpenGL() {
    std::cout << "Loading OpenGL functions..." << std::endl;
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    // Temporarily disable face culling to see if that's the issue
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
    
    std::cout << "OpenGL initialized successfully" << std::endl;
    return true;
}

void App::run() {
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        processInput();
        update(deltaTime);
        render();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void App::processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
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

void App::update(float deltaTime) {
    // Update camera direction vectors based on yaw and pitch
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void App::render() {
    // Start rendering to low-res pixel buffer
    pixelRenderer->beginPixelRender();
    
    // Create view and projection matrices
    // Use pixel buffer dimensions for proper aspect ratio
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                          (float)pixelRenderer->getPixelWidth() / (float)pixelRenderer->getPixelHeight(), 
                                          0.1f, 100.0f);
    
    // Render map if loaded
    if (currentMap && mapRenderer) {
        const auto& worldSettings = currentMap->getWorldSettings();
        glClearColor(worldSettings.backgroundColor.r, 
                     worldSettings.backgroundColor.g, 
                     worldSettings.backgroundColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        mapRenderer->render(view, projection);
    }
    
    // End pixel rendering and display to screen
    pixelRenderer->endPixelRender(WINDOW_WIDTH, WINDOW_HEIGHT);
}

void App::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void App::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (!app) return;
    
    float xposf = static_cast<float>(xpos);
    float yposf = static_cast<float>(ypos);
    
    if (app->firstMouse) {
        app->lastX = xposf;
        app->lastY = yposf;
        app->firstMouse = false;
    }
    
    float xoffset = xposf - app->lastX;
    float yoffset = app->lastY - yposf; // Reversed since y coordinates go from bottom to top
    app->lastX = xposf;
    app->lastY = yposf;
    
    xoffset *= MOUSE_SENSITIVITY;
    yoffset *= MOUSE_SENSITIVITY;
    
    app->yaw += xoffset;
    app->pitch += yoffset;
    
    // Constrain pitch
    if (app->pitch > 89.0f)
        app->pitch = 89.0f;
    if (app->pitch < -89.0f)
        app->pitch = -89.0f;
}

void App::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // For now, we don't need scroll functionality, but keep the callback
    (void)window;
    (void)xoffset; 
    (void)yoffset;
}

bool App::loadMap(const std::string& mapFile) {
    std::cout << "Attempting to load map: " << mapFile << std::endl;
    
    if (!currentMap->loadFromFile(mapFile)) {
        std::cerr << "Failed to load map: " << mapFile << std::endl;
        return false;
    }
    
    std::cout << "Map loaded successfully. Brushes: " << currentMap->getBrushes().size() 
              << ", Entities: " << currentMap->getEntities().size() 
              << ", Lights: " << currentMap->getLights().size() << std::endl;
    
    if (!mapRenderer->loadMap(*currentMap)) {
        std::cerr << "Failed to load map into renderer" << std::endl;
        return false;
    }
    
    std::cout << "Map loaded into renderer successfully" << std::endl;
    
    // Position camera at player start if available
    Entity* playerStart = currentMap->getPlayerStart();
    if (playerStart) {
        cameraPos = playerStart->position;
        std::cout << "Player start position: " << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << std::endl;
    } else {
        std::cout << "No player start found, using default camera position" << std::endl;
    }
    
    return true;
}

} // namespace silic2