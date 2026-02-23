#pragma once

#include <glm/glm.hpp>
#include "engine/collision.h"
#include "engine/game_config.h"
#include <memory>

struct GLFWwindow;

namespace silic2 {

class Camera;
class Map;

// Player states
enum class PlayerState {
    IDLE,
    WALKING,
    RUNNING,
    CROUCHING,
    JUMPING,
    FALLING,
    SLIDING
};


class Player {
public:
    Player(const glm::vec3& startPosition = glm::vec3(0.0f, 2.0f, 0.0f));
    ~Player() = default;
    
    // Update player state
    void update(float deltaTime, const Map* map);
    
    // Process input
    void processInput(GLFWwindow* window, Camera* camera, float deltaTime);
    void processMouseMovement(Camera* camera, float xoffset, float yoffset);
    
    // Get player position
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getEyePosition() const { 
        const auto& config = GameConfig::getInstance().player;
        return position + glm::vec3(0.0f, config.eyeHeight + currentCameraHeight, 0.0f); 
    }
    
    // Get player velocity
    glm::vec3 getVelocity() const { return velocity; }
    
    // Get player state
    PlayerState getState() const { return state; }
    bool isOnGround() const { return onGround; }
    bool isSprinting() const { return sprinting; }
    bool isGodMode() const { return GameConfig::getInstance().player.enableGodMode; }
    
    // Set position
    void setPosition(const glm::vec3& pos);
    
    // Get player AABB
    AABB getAABB() const;
    
    
    // Health
    void  takeDamage(float amount);
    float getHp()    const { return currentHp; }
    float getMaxHp() const { return GameConfig::getInstance().player.maxHp; }
    bool  isDead()   const { return currentHp <= 0.0f; }

    // Get current FOV for camera
    float getCurrentFov() const { return currentFov; }
    
    // Get camera effects
    glm::vec3 getCameraOffset() const { return headBobOffset + cameraShakeOffset; }
    
private:
    // Player attributes
    glm::vec3 position;
    glm::vec3 velocity;
    PlayerState state;
    bool onGround;
    bool sprinting = false;
    
    // Get configuration from GameConfig
    const PlayerConfig& getConfig() const { return GameConfig::getInstance().player; }
    
    // Input state
    bool jumpPressed = false;
    bool wasJumpPressed = false;
    bool godModePressed = false;
    bool wasGodModePressed = false;
    bool crouchPressed = false;
    bool wasCrouchPressed = false;
    bool shiftPressed = false;
    bool wasShiftPressed = false;
    
    // FOV management
    float currentFov = 45.0f;
    float targetFov = 45.0f;
    
    // Camera bobbing/shake effects
    float bobTime = 0.0f;
    glm::vec3 headBobOffset = glm::vec3(0.0f);
    glm::vec3 cameraShakeOffset = glm::vec3(0.0f);
    
    // Camera height adjustment for states
    float currentCameraHeight = 0.0f;       // Current height offset from normal eye height
    float targetCameraHeight = 0.0f;        // Target height offset
    float cameraHeightTransitionSpeed = 8.0f; // Speed of height transitions
    
    // Health
    float currentHp = 100.0f;

    // Momentum/Inertia system
    glm::vec3 momentum = glm::vec3(0.0f);  // Persistent velocity that gradually decays
    float airControl = 0.4f;               // How much control player has in air
    float groundFriction = 25.0f;          // Very high friction = immediate stop
    float airResistance = 0.8f;            // How quickly momentum decays in air (lower = more gliding)
    
    // Sliding system
    bool sliding = false;                   // Currently sliding
    float slideTime = 0.0f;                // Time spent sliding
    glm::vec3 slideDirection = glm::vec3(0.0f); // Direction of slide
    
    // Crouching system
    bool crouching = false;                 // Currently crouching
    
    // Sprint toggle system
    bool sprintToggled = false;             // Sprint state (toggled by Shift)
    
    // Physics update
    void updatePhysics(float deltaTime, const Map* map);
    
    // Handle collision with map
    void handleMapCollision(const Map* map);
    
    // Check if on ground
    bool checkGroundCollision(const Map* map);
    
    // Move with collision detection
    glm::vec3 moveWithCollision(const glm::vec3& movement, const Map* map);
    
    // Get movement input
    glm::vec3 getMovementInput(GLFWwindow* window, Camera* camera, float deltaTime);
    
    // Update FOV based on sprinting state
    void updateFov(float deltaTime);
    
    // Update camera effects
    void updateCameraEffects(float deltaTime);
    
    // Update camera height based on state
    void updateCameraHeight(float deltaTime);
};

} // namespace silic2