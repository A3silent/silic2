#pragma once

#include <glm/glm.hpp>
#include <string>

namespace silic2 {

struct WindowConfig {
    int width = 1280;
    int height = 720;
    std::string title = "Silic2";
    bool fullscreen = false;
};

struct RenderConfig {
    int pixelWidth = 320;
    int pixelHeight = 200;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    bool enableVSync = true;
    bool enableDepthTest = true;
};

struct PlayerConfig {
    // Movement
    float moveSpeed = 5.0f;
    float sprintSpeed = 10.0f;
    float jumpVelocity = 8.0f;
    float gravity = -20.0f;
    float maxFallSpeed = -50.0f;
    
    // Physical properties
    float height = 1.8f;
    float radius = 0.3f;
    float eyeHeight = 1.6f;
    float stepHeight = 0.3f;
    float groundCheckDistance = 0.1f;
    
    // Camera/View settings
    float mouseSensitivity = 0.03f;
    float normalFov = 45.0f;
    float sprintFov = 60.0f;
    float slideFov = 75.0f;             // Higher FOV during slide
    float fovTransitionSpeed = 5.0f;
    
    // Sliding settings
    float slideSpeed = 15.0f;           // Initial slide speed (faster than sprint)
    float slideFriction = 4.0f;         // Friction during slide (reduced for smoother flow)
    float maxSlideTime = 2.0f;          // Maximum slide duration
    
    // Crouching settings
    float crouchSpeed = 2.0f;           // Movement speed while crouching
    float crouchHeight = 0.9f;          // Height when crouching (multiplier)
    float crouchCameraOffset = -0.6f;   // Camera height offset when crouching
    float slideCameraOffset = -0.8f;    // Camera height offset when sliding
    
    // God mode settings
    float godModeSpeed = 15.0f;         // Flight speed in god mode
    bool enableGodMode = false;         // God mode toggle
};

struct CameraConfig {
    float yaw = -90.0f;
    float pitch = 0.0f;
    float minPitch = -89.0f;
    float maxPitch = 89.0f;
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
};

struct EffectsConfig {
    bool enableGroundParticles = true;  // Enable/disable ground fire particles
    float groundParticleIntensity = 3.0f;  // Particle brightness multiplier
    float groundParticleEmissionRate = 100.0f;  // Particles per second
};

class GameConfig {
public:
    static GameConfig& getInstance();
    
    // Configuration sections
    WindowConfig window;
    RenderConfig render;
    PlayerConfig player;
    CameraConfig camera;
    EffectsConfig effects;
    
    // Load/Save configuration
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename) const;
    
    // Reset to defaults
    void resetToDefaults();

private:
    GameConfig() = default;
    GameConfig(const GameConfig&) = delete;
    GameConfig& operator=(const GameConfig&) = delete;
};

} // namespace silic2