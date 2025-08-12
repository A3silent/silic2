#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "app.h"
#include "map.h"
#include "map_renderer.h"
#include "pixel_renderer.h"
#include "player.h"
#include "weapon.h"
#include "particle_system.h"
#include "groundparticle.h"
#include "game_config.h"
#include <iostream>
#include <stdexcept>

namespace silic2 {

App::App() : window(nullptr) {
             
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
        // Initialize with configured resolution
        const auto& renderConfig = GameConfig::getInstance().render;
        if (!pixelRenderer->init(renderConfig.pixelWidth, renderConfig.pixelHeight)) {
            throw std::runtime_error("Failed to initialize PixelRenderer");
        }
        std::cout << "PixelRenderer created successfully" << std::endl;
        
        // Create camera
        camera = std::make_unique<Camera>(glm::vec3(0.0f, 5.0f, 5.0f));
        const auto& windowConfig = GameConfig::getInstance().window;
        camera->setLastMousePos(windowConfig.width / 2.0f, windowConfig.height / 2.0f);
        
        // Create player
        player = std::make_unique<Player>(glm::vec3(0.0f, 2.0f, 0.0f));
        
        // Create weapon
        weapon = std::make_unique<Weapon>();
        weapon->init();
        
        // Create enhanced ground particle system
        groundParticles = createEnhancedGroundParticleSystem(2000);
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
    
    const auto& config = GameConfig::getInstance().window;
    window = glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, nullptr);
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
    
    const auto& config = GameConfig::getInstance().window;
    glViewport(0, 0, config.width, config.height);
    
    const auto& renderConfig = GameConfig::getInstance().render;
    if (renderConfig.enableDepthTest) {
        glEnable(GL_DEPTH_TEST);
    }
    
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
    
    // Player movement and input
    if (player) {
        player->processInput(window, camera.get(), deltaTime);
    }
    
    // Weapon input - Left mouse button to fire
    static bool wasMousePressed = false;
    bool isMousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (isMousePressed && !wasMousePressed && weapon) {
        weapon->fire(*camera);
    }
    wasMousePressed = isMousePressed;
}

void App::update(float deltaTime) {
    // Update player
    if (player) {
        player->update(deltaTime, currentMap.get());
        
        // Update camera to follow player with bobbing/shake effects
        glm::vec3 eyePos = player->getEyePosition();
        glm::vec3 cameraOffset = player->getCameraOffset();
        camera->setPosition(eyePos + cameraOffset);
    }
    
    // Update camera
    camera->update();
    
    // Update weapon
    if (weapon) {
        weapon->update(deltaTime, currentMap.get());
    }
    
    // Update ground particle system
    if (groundParticles) {
        groundParticles->update(deltaTime);
    }
}

void App::render() {
    // Start rendering to low-res pixel buffer
    pixelRenderer->beginPixelRender();
    
    // Create view and projection matrices
    // Use pixel buffer dimensions for proper aspect ratio
    glm::mat4 view = camera->getViewMatrix();
    
    // Use player's current FOV if player exists
    glm::mat4 projection;
    if (player) {
        projection = camera->getProjectionMatrix(
            (float)pixelRenderer->getPixelWidth() / (float)pixelRenderer->getPixelHeight(),
            player->getCurrentFov());
    } else {
        projection = camera->getProjectionMatrix(
            (float)pixelRenderer->getPixelWidth() / (float)pixelRenderer->getPixelHeight());
    }
    
    // Render map if loaded
    if (currentMap && mapRenderer) {
        const auto& worldSettings = currentMap->getWorldSettings();
        glClearColor(worldSettings.backgroundColor.r, 
                     worldSettings.backgroundColor.g, 
                     worldSettings.backgroundColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Clear dynamic lights before rendering
        mapRenderer->clearDynamicLights();
        
        // Add bullet lights to map renderer
        if (weapon) {
            auto bulletLights = weapon->getActiveLights();
            for (const auto& [pos, color] : bulletLights) {
                // color already includes intensity, so set intensity to 1.0 here
                mapRenderer->addDynamicLight(pos, color, 1.0f, 3.0f);
            }
        }
        
        mapRenderer->render(view, projection);
    }
    
    // Render weapon bullets
    if (weapon) {
        weapon->render(view, projection);
    }
    
    // Render ground particle system if enabled
    const auto& effectsConfig = GameConfig::getInstance().effects;
    if (groundParticles && effectsConfig.enableGroundParticles) {
        groundParticles->render(view, projection);
    }
    
    // End pixel rendering and display to screen
    const auto& config = GameConfig::getInstance().window;
    pixelRenderer->endPixelRender(config.width, config.height);
}

void App::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void App::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (!app) return;
    
    float xposf = static_cast<float>(xpos);
    float yposf = static_cast<float>(ypos);
    
    if (app->camera->isFirstMouse()) {
        app->camera->setLastMousePos(xposf, yposf);
        app->camera->setFirstMouse(false);
    }
    
    float xoffset = xposf - app->camera->getLastX();
    float yoffset = app->camera->getLastY() - yposf; // Reversed since y coordinates go from bottom to top
    app->camera->setLastMousePos(xposf, yposf);
    
    // Use player's mouse handling if player exists, otherwise use camera directly
    if (app->player) {
        app->player->processMouseMovement(app->camera.get(), xoffset, yoffset);
    } else {
        app->camera->processMouseMovement(xoffset, yoffset);
    }
}

void App::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (!app || !app->camera) return;
    
    app->camera->processMouseScroll(static_cast<float>(yoffset));
    (void)xoffset; // Unused
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
    
    // Position player at player start if available
    Entity* playerStart = currentMap->getPlayerStart();
    if (playerStart) {
        if (player) {
            player->setPosition(playerStart->position);
        }
        camera->setPosition(playerStart->position + glm::vec3(0.0f, 1.6f, 0.0f));
        std::cout << "Player start position: " << playerStart->position.x << ", " 
                  << playerStart->position.y << ", " << playerStart->position.z << std::endl;
    } else {
        std::cout << "No player start found, using default position" << std::endl;
    }
    
    // Initialize ground particle system with the loaded map
    const auto& effectsConfig = GameConfig::getInstance().effects;
    if (groundParticles && effectsConfig.enableGroundParticles) {
        groundParticles->initialize(*currentMap);
        groundParticles->setEmissionRate(effectsConfig.groundParticleEmissionRate);
        groundParticles->setFireIntensity(effectsConfig.groundParticleIntensity);
        groundParticles->setEnabled(true);
        std::cout << "Ground particle system initialized with map floor data" << std::endl;
    } else if (groundParticles) {
        groundParticles->setEnabled(false);
        std::cout << "Ground particle system disabled by config" << std::endl;
    }
    
    return true;
}

} // namespace silic2