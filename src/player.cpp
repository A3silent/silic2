#include "player.h"
#include "camera.h"
#include "map.h"
#include "game_config.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>

namespace silic2 {

Player::Player(const glm::vec3& startPosition) 
    : position(startPosition), 
      velocity(0.0f),
      state(PlayerState::IDLE),
      onGround(false) {
}

void Player::update(float deltaTime, const Map* map) {
    // Update physics (skip if in god mode)
    if (!isGodMode()) {
        updatePhysics(deltaTime, map);
    }
    
    // Update FOV
    updateFov(deltaTime);
    
    // Update state
    if (isGodMode()) {
        // In god mode, set state based on movement
        float totalSpeed = glm::length(velocity);
        if (totalSpeed > 0.1f) {
            state = PlayerState::RUNNING; // Flying state
        } else {
            state = PlayerState::IDLE;
        }
    } else {
        if (onGround) {
            float horizontalSpeed = glm::length(glm::vec2(velocity.x, velocity.z));
            if (horizontalSpeed > 0.1f) {
                if (sprinting) {
                    state = PlayerState::RUNNING;
                } else {
                    state = PlayerState::WALKING;
                }
            } else {
                state = PlayerState::IDLE;
            }
        } else {
            if (velocity.y > 0) {
                state = PlayerState::JUMPING;
            } else {
                state = PlayerState::FALLING;
            }
        }
    }
}

void Player::processInput(GLFWwindow* window, Camera* camera, float deltaTime) {
    // God mode toggle
    wasGodModePressed = godModePressed;
    godModePressed = glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS;
    
    // Toggle god mode when G is first pressed
    if (godModePressed && !wasGodModePressed) {
        auto& config = GameConfig::getInstance().player;
        config.enableGodMode = !config.enableGodMode;
        std::cout << "God mode " << (config.enableGodMode ? "enabled" : "disabled") << std::endl;
    }
    
    // Get movement input
    glm::vec3 movement = getMovementInput(window, camera, deltaTime);
    
    if (isGodMode()) {
        // In god mode, apply movement directly to position for free flight
        position += movement * deltaTime;
        velocity = glm::vec3(0.0f); // Reset velocity in god mode
    } else {
        // Normal movement physics
        // Apply movement (allow air control)
        if (onGround || true) { // Allow air control
            velocity.x = movement.x;
            velocity.z = movement.z;
        }
        
        // Jump input
        wasJumpPressed = jumpPressed;
        jumpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        
        // Jump only when key is first pressed and on ground
        if (jumpPressed && !wasJumpPressed && onGround) {
            const auto& config = GameConfig::getInstance().player;
            velocity.y = config.jumpVelocity;
            onGround = false;
        }
    }
}

void Player::setPosition(const glm::vec3& pos) {
    position = pos;
}

AABB Player::getAABB() const {
    const auto& config = GameConfig::getInstance().player;
    glm::vec3 halfSize(config.radius, config.height * 0.5f, config.radius);
    glm::vec3 center = position + glm::vec3(0.0f, config.height * 0.5f, 0.0f);
    return AABB::fromCenterHalfSize(center, halfSize);
}

void Player::updatePhysics(float deltaTime, const Map* map) {
    // Apply gravity
    if (!onGround) {
        const auto& config = GameConfig::getInstance().player;
        velocity.y += config.gravity * deltaTime;
        velocity.y = std::max(velocity.y, config.maxFallSpeed);
    }
    
    // Calculate desired movement
    glm::vec3 movement = velocity * deltaTime;
    
    // Move with collision detection
    glm::vec3 actualMovement = moveWithCollision(movement, map);
    
    // Update position
    position += actualMovement;
    
    // If vertical movement was blocked, reset vertical velocity
    if (std::abs(movement.y - actualMovement.y) > 0.001f) {
        velocity.y = 0.0f;
    }
    
    // Check if on ground
    onGround = checkGroundCollision(map);
}

void Player::handleMapCollision(const Map* map) {
    if (!map) return;
    
    AABB playerBox = getAABB();
    const auto& brushes = map->getBrushes();
    
    for (const auto& brush : brushes) {
        // Calculate brush AABB
        if (brush.vertices.empty()) continue;
        
        glm::vec3 brushMin = brush.vertices[0];
        glm::vec3 brushMax = brush.vertices[0];
        
        for (const auto& vertex : brush.vertices) {
            brushMin = glm::min(brushMin, vertex);
            brushMax = glm::max(brushMax, vertex);
        }
        
        AABB brushBox(brushMin, brushMax);
        
        // Check collision and resolve
        if (CollisionSystem::checkAABB(playerBox, brushBox)) {
            glm::vec3 resolution = CollisionSystem::resolveAABBCollision(playerBox, brushBox);
            position += resolution;
            
            // If pushed upward, we're standing on ground
            if (resolution.y > 0) {
                onGround = true;
                velocity.y = 0.0f;
            }
        }
    }
}

bool Player::checkGroundCollision(const Map* map) {
    if (!map) return false;
    
    // Create a slightly downward-extending detection box
    AABB groundCheckBox = getAABB();
    const auto& config = GameConfig::getInstance().player;
    groundCheckBox.min.y -= config.groundCheckDistance;
    
    const auto& brushes = map->getBrushes();
    
    for (const auto& brush : brushes) {
        if (brush.vertices.empty()) continue;
        
        glm::vec3 brushMin = brush.vertices[0];
        glm::vec3 brushMax = brush.vertices[0];
        
        for (const auto& vertex : brush.vertices) {
            brushMin = glm::min(brushMin, vertex);
            brushMax = glm::max(brushMax, vertex);
        }
        
        AABB brushBox(brushMin, brushMax);
        
        if (CollisionSystem::checkAABB(groundCheckBox, brushBox)) {
            // Additional check: ensure brush is below player
            if (brushBox.max.y <= position.y + 0.1f) {
                return true;
            }
        }
    }
    
    return false;
}

glm::vec3 Player::moveWithCollision(const glm::vec3& movement, const Map* map) {
    if (!map || isGodMode()) return movement; // Skip collision in god mode
    
    glm::vec3 finalMovement = movement;
    AABB originalBox = getAABB();
    
    const auto& brushes = map->getBrushes();
    
    const auto& config = GameConfig::getInstance().player;
    
    // Handle movement for each axis separately
    for (int axis = 0; axis < 3; ++axis) {
        glm::vec3 axisMovement(0.0f);
        axisMovement[axis] = movement[axis];
        
        if (std::abs(axisMovement[axis]) < 0.0001f) continue;
        
        // Create moved box
        AABB movedBox = originalBox;
        movedBox.min += axisMovement;
        movedBox.max += axisMovement;
        
        bool collision = false;
        
        // Check collision with all brushes
        for (const auto& brush : brushes) {
            if (brush.vertices.empty()) continue;
            
            glm::vec3 brushMin = brush.vertices[0];
            glm::vec3 brushMax = brush.vertices[0];
            
            for (const auto& vertex : brush.vertices) {
                brushMin = glm::min(brushMin, vertex);
                brushMax = glm::max(brushMax, vertex);
            }
            
            AABB brushBox(brushMin, brushMax);
            
            if (CollisionSystem::checkAABB(movedBox, brushBox)) {
                collision = true;
                
                // Try step climbing (only for non-Y axis and when on ground)
                if (axis != 1 && onGround && brushBox.max.y - originalBox.min.y <= config.stepHeight) {
                    // Try moving upward
                    AABB stepBox = originalBox;
                    stepBox.min.y = brushBox.max.y;
                    stepBox.max.y = stepBox.min.y + config.height;
                    stepBox.min += axisMovement;
                    stepBox.max += axisMovement;
                    
                    // Check if there's still collision after stepping up
                    bool canStep = true;
                    for (const auto& otherBrush : brushes) {
                        if (otherBrush.vertices.empty()) continue;
                        
                        glm::vec3 otherMin = otherBrush.vertices[0];
                        glm::vec3 otherMax = otherBrush.vertices[0];
                        
                        for (const auto& vertex : otherBrush.vertices) {
                            otherMin = glm::min(otherMin, vertex);
                            otherMax = glm::max(otherMax, vertex);
                        }
                        
                        AABB otherBox(otherMin, otherMax);
                        if (CollisionSystem::checkAABB(stepBox, otherBox)) {
                            canStep = false;
                            break;
                        }
                    }
                    
                    if (canStep) {
                        finalMovement.y = brushBox.max.y - originalBox.min.y + 0.01f;
                        collision = false;
                    }
                }
                
                if (collision) break;
            }
        }
        
        if (!collision) {
            originalBox.min += axisMovement;
            originalBox.max += axisMovement;
        } else {
            finalMovement[axis] = 0.0f;
        }
    }
    
    return finalMovement;
}

glm::vec3 Player::getMovementInput(GLFWwindow* window, Camera* camera, float deltaTime) {
    (void)deltaTime; // Suppress unused parameter warning
    glm::vec3 movement(0.0f);
    
    // Get camera vectors
    glm::vec3 forward = camera->getFront();
    glm::vec3 right = camera->getRight();
    glm::vec3 up = camera->getUp();
    
    // In god mode, use full 3D movement; otherwise flatten Y component
    if (!isGodMode()) {
        forward.y = 0.0f;
        forward = glm::normalize(forward);
        right.y = 0.0f;
        right = glm::normalize(right);
    }
    
    // Check for sprint input (Shift + W)
    bool shiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    bool wPressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    
    sprinting = shiftPressed && wPressed;
    
    // Update target FOV based on sprinting
    const auto& config = GameConfig::getInstance().player;
    if (sprinting) {
        targetFov = config.sprintFov;
    } else {
        targetFov = config.normalFov;
    }
    
    // WASD movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        movement += forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        movement -= forward;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        movement -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        movement += right;
    
    // God mode: add vertical movement with Space/Shift
    if (isGodMode()) {
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            movement += up;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            movement -= up;
    }
    
    // Normalize and apply speed
    if (glm::length(movement) > 0.0f) {
        const auto& config = GameConfig::getInstance().player;
        float currentSpeed;
        
        if (isGodMode()) {
            currentSpeed = config.godModeSpeed;
        } else {
            currentSpeed = sprinting ? config.sprintSpeed : config.moveSpeed;
        }
        
        movement = glm::normalize(movement) * currentSpeed;
    }
    
    return movement;
}

void Player::processMouseMovement(Camera* camera, float xoffset, float yoffset) {
    if (!camera) return;
    
    // Apply player's mouse sensitivity
    const auto& config = GameConfig::getInstance().player;
    xoffset *= config.mouseSensitivity;
    yoffset *= config.mouseSensitivity;
    
    // Forward to camera
    camera->processMouseMovement(xoffset, yoffset);
}

void Player::updateFov(float deltaTime) {
    // Smoothly interpolate FOV
    float fovDifference = targetFov - currentFov;
    if (std::abs(fovDifference) > 0.1f) {
        const auto& config = GameConfig::getInstance().player;
        currentFov += fovDifference * config.fovTransitionSpeed * deltaTime;
    } else {
        currentFov = targetFov;
    }
}

} // namespace silic2