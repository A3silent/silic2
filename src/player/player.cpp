#include "player/player.h"
#include "engine/camera.h"
#include "engine/map.h"
#include "engine/game_config.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cmath>

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
    
    // Update camera effects (bobbing, shaking)
    updateCameraEffects(deltaTime);
    
    // Update camera height for crouching/sliding
    updateCameraHeight(deltaTime);
    
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
            if (sliding) {
                state = PlayerState::SLIDING;
            } else if (crouching) {
                state = PlayerState::CROUCHING;
            } else {
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
    }
    
    // Get movement input
    glm::vec3 movement = getMovementInput(window, camera, deltaTime);
    
    if (isGodMode()) {
        // In god mode, apply movement directly to position for free flight
        position += movement * deltaTime;
        velocity = glm::vec3(0.0f); // Reset velocity in god mode
        momentum = glm::vec3(0.0f); // Reset momentum in god mode
    } else {
        // Normal movement physics with momentum system
        if (onGround) {
            if (sliding) {
                // During slide: no direct control, momentum controls movement
                velocity.x = 0.0f;
                velocity.z = 0.0f;
            } else {
                // On ground: direct control with strong friction to cancel momentum when needed
                velocity.x = movement.x;
                velocity.z = movement.z;
                
                // When actively moving, override momentum for responsive control
                if (glm::length(glm::vec2(movement.x, movement.z)) > 0.1f) {
                    // Store current movement as momentum for potential jumps
                    momentum.x = movement.x;
                    momentum.z = movement.z;
                }
                // When not moving, let friction naturally decay momentum (handled in updatePhysics)
            }
        } else {
            // In air: preserve momentum from jump, add limited control
            glm::vec2 airInput(movement.x, movement.z);
            glm::vec2 airControlVector = airInput * airControl;
            
            // Don't override momentum in air, just add air control on top
            velocity.x = airControlVector.x; // Small direct control
            velocity.z = airControlVector.y;
            
            // Keep momentum separate - don't modify it directly from input in air
        }
        
        // Ctrl input for crouching/sliding
        wasCrouchPressed = crouchPressed;
        crouchPressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
        
        // Handle Ctrl press
        if (crouchPressed && !wasCrouchPressed && onGround) {
            if (sliding) {
                // End sliding when Ctrl pressed during slide - convert momentum to sprint velocity
                sliding = false;
                slideTime = 0.0f;
                
                // Calculate current momentum speed
                float momentumSpeed = glm::length(glm::vec2(momentum.x, momentum.z));
                const auto& config = GameConfig::getInstance().player;
                
                // If momentum is strong enough, enable sprint and convert to velocity
                if (momentumSpeed >= config.sprintSpeed && momentumSpeed > 0.1f) {
                    sprintToggled = true;  // Enable sprint
                    // Convert momentum back to normal velocity for sprinting
                    glm::vec3 momentumDir = glm::normalize(glm::vec3(momentum.x, 0.0f, momentum.z));
                    velocity.x = momentumDir.x * config.sprintSpeed;
                    velocity.z = momentumDir.z * config.sprintSpeed;
                    momentum.x = velocity.x;  // Set momentum to sprint speed
                    momentum.z = velocity.z;
                } else {
                    // Not fast enough for sprint, just end slide normally
                    velocity.x = momentum.x;
                    velocity.z = momentum.z;
                }
            } else {
                float currentSpeed = glm::length(glm::vec2(velocity.x, velocity.z));
                
                if (currentSpeed > 5.0f && !crouching) { // Need good speed for sliding
                    // Start sliding
                    const auto& config = GameConfig::getInstance().player;
                    sliding = true;
                    slideTime = 0.0f;
                    crouching = false;
                    
                    // Use current movement direction, not just velocity
                    glm::vec3 movementDir = glm::vec3(movement.x, 0.0f, movement.z);
                    if (glm::length(movementDir) > 0.1f) {
                        slideDirection = glm::normalize(movementDir);
                    } else {
                        slideDirection = glm::normalize(glm::vec3(velocity.x, 0.0f, velocity.z));
                    }
                    
                    // Set strong initial slide momentum
                    momentum.x = slideDirection.x * config.slideSpeed;
                    momentum.z = slideDirection.z * config.slideSpeed;
                } else if (!sliding) {
                    // Start crouching
                    crouching = true;
                }
            }
        }
        
        // Stop crouching when Ctrl is released (but not sliding)
        if (!crouchPressed && wasCrouchPressed && crouching && !sliding) {
            crouching = false;
        }
        
        // Sprint toggle with Shift
        wasShiftPressed = shiftPressed;
        shiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
        
        // Toggle sprint when Shift is first pressed (only if not crouching or sliding)
        if (shiftPressed && !wasShiftPressed && !crouching && !sliding) {
            sprintToggled = !sprintToggled;
        }
        
        // Update sprinting state - only sprint if toggled and moving forward
        bool wPressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        sprinting = sprintToggled && wPressed && !crouching && !sliding;
        
        // Jump input
        wasJumpPressed = jumpPressed;
        jumpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        
        // Jump interrupts slide or normal jump
        if (jumpPressed && !wasJumpPressed && onGround) {
            const auto& config = GameConfig::getInstance().player;
            velocity.y = config.jumpVelocity;
            onGround = false;
            
            // If sliding, interrupt slide
            if (sliding) {
                sliding = false;
                slideTime = 0.0f;
            }
            
            // Stop crouching when jumping
            if (crouching) {
                crouching = false;
            }
            
            // Preserve horizontal momentum when jumping
            // Don't reset momentum - let it carry through the jump
        }
    }
}

void Player::takeDamage(float amount) {
    currentHp -= amount;
    if (currentHp < 0.0f) currentHp = 0.0f;
}

void Player::setPosition(const glm::vec3& pos) {
    position = pos;
    // Restore full HP when repositioned (respawn/map load)
    currentHp = GameConfig::getInstance().player.maxHp;
}

void Player::respawn(const glm::vec3& pos) {
    position  = pos;
    velocity  = glm::vec3(0.0f);
    momentum  = glm::vec3(0.0f);
    currentHp = GameConfig::getInstance().player.maxHp;
    state     = PlayerState::IDLE;
    sliding   = false;
    crouching = false;
    sprinting = false;
    sprintToggled = false;
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
    
    // Update sliding
    if (sliding) {
        const auto& config = GameConfig::getInstance().player;
        slideTime += deltaTime;
        
        // Check if slide should end
        if (slideTime >= config.maxSlideTime || !onGround) {
            sliding = false;
            sprintToggled = false; // Reset sprint toggle to return to walking
            slideTime = 0.0f;
        } else {
            // Apply slide friction - gradually reduce momentum
            glm::vec2 horizontalMomentum(momentum.x, momentum.z);
            float currentSpeed = glm::length(horizontalMomentum);
            
            if (currentSpeed > 0.3f) {  // Only apply friction if still moving at reasonable speed (reduced threshold)
                glm::vec2 frictionForce = -glm::normalize(horizontalMomentum) * config.slideFriction * deltaTime;
                momentum.x += frictionForce.x;
                momentum.z += frictionForce.y;
            } else if (currentSpeed <= 0.3f) {
                // Slide stopped naturally due to low speed
                momentum.x = 0.0f;
                momentum.z = 0.0f;
                sliding = false;
                sprintToggled = false; // Reset sprint toggle to return to walking
                sprinting = false; // Stop running if slide ends
                slideTime = 0.0f;
            }
        }
    }
    
    // Apply momentum decay based on whether on ground or in air
    if (onGround && !sliding) {
        // On ground: strong friction but only when not actively moving
        glm::vec2 inputVel(velocity.x, velocity.z);
        float inputSpeed = glm::length(inputVel);
        
        if (inputSpeed < 0.1f) {
            // No input - apply strong friction to momentum
            glm::vec2 horizontalMomentum(momentum.x, momentum.z);
            float currentSpeed = glm::length(horizontalMomentum);
            if (currentSpeed > 0.01f) { // Lower threshold for friction
                glm::vec2 frictionForce = -glm::normalize(horizontalMomentum) * groundFriction * deltaTime;
                if (glm::length(frictionForce) > currentSpeed) {
                    momentum.x = 0.0f;
                    momentum.z = 0.0f;
                } else {
                    momentum.x += frictionForce.x;
                    momentum.z += frictionForce.y;
                }
            } else {
                // Very small momentum - just clear it
                momentum.x = 0.0f;
                momentum.z = 0.0f;
            }
        }
        // When moving, momentum is controlled by input (set in processInput)
    } else {
        // In air: apply air resistance to all momentum
        momentum *= (1.0f - airResistance * deltaTime);
    }
    
    // Combine velocity (immediate input) with momentum (persistent movement)
    glm::vec3 totalVelocity = velocity;
    
    // Add momentum - but handle ground vs air differently
    if (onGround) {
        // On ground: use direct input OR momentum, whichever is stronger
        glm::vec2 inputSpeed(velocity.x, velocity.z);
        glm::vec2 momentumSpeed(momentum.x, momentum.z);
        
        if (glm::length(inputSpeed) > 0.1f) {
            // Active input overrides momentum for responsive control
            totalVelocity.x = velocity.x;
            totalVelocity.z = velocity.z;
        } else {
            // No input: use only momentum (sliding)
            totalVelocity.x = momentum.x;
            totalVelocity.z = momentum.z;
        }
    } else {
        // In air: always combine momentum with air control
        totalVelocity.x += momentum.x;
        totalVelocity.z += momentum.z;
    }
    
    // Calculate desired movement
    glm::vec3 movement = totalVelocity * deltaTime;
    
    // Move with collision detection
    glm::vec3 actualMovement = moveWithCollision(movement, map);
    
    // Update position
    position += actualMovement;
    
    // If horizontal movement was blocked, reduce momentum in that direction
    if (std::abs(movement.x - actualMovement.x) > 0.001f) {
        momentum.x *= 0.5f;  // Reduce X momentum on collision
    }
    if (std::abs(movement.z - actualMovement.z) > 0.001f) {
        momentum.z *= 0.5f;  // Reduce Z momentum on collision
    }
    
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
    
    // Sprinting is now handled in processInput function
    
    // Update target FOV based on state
    const auto& config = GameConfig::getInstance().player;
    if (sliding) {
        targetFov = config.slideFov;  // Highest FOV for sliding
    } else if (sprinting) {
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
        } else if (crouching) {
            currentSpeed = config.crouchSpeed;
        } else if (sprinting) {
            currentSpeed = config.sprintSpeed;
        } else {
            currentSpeed = config.moveSpeed;
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

void Player::updateCameraEffects(float deltaTime) {
    // Reset effects
    headBobOffset = glm::vec3(0.0f);
    cameraShakeOffset = glm::vec3(0.0f);
    
    // Only apply effects when not in god mode and on ground
    if (isGodMode() || !onGround) {
        bobTime = 0.0f;
        return;
    }
    
    float horizontalSpeed = glm::length(glm::vec2(velocity.x, velocity.z));
    
    // For sliding, use momentum speed instead of direct velocity
    if (sliding) {
        horizontalSpeed = glm::length(glm::vec2(momentum.x, momentum.z));
    }
    
    // Head bobbing when moving
    if (horizontalSpeed > 0.1f || sliding) {
        // Different bob speeds and intensities for walking vs running vs sliding
        float bobSpeed, bobIntensity, sideIntensity;
        
        if (sliding) {
            // Sliding effects - more intense and chaotic
            bobSpeed = 20.0f;      // Very fast motion for sliding
            bobIntensity = 0.15f;  // Strong vertical shake
            sideIntensity = 0.08f; // Strong side shake
        } else if (crouching) {
            // Crouching effects - minimal and subtle
            bobSpeed = 4.0f;       // Very slow bobbing when crouching
            bobIntensity = 0.02f;  // Minimal vertical bob
            sideIntensity = 0.01f; // Very subtle side sway
        } else if (sprinting) {
            bobSpeed = 14.0f;      // Fast bobbing when sprinting
            bobIntensity = 0.10f;   // Moderate vertical bob
            sideIntensity = 0.05f;  // Moderate side-to-side sway
        } else {
            bobSpeed = 8.0f;       // Slower bobbing when walking
            bobIntensity = 0.04f;   // Less vertical bob
            sideIntensity = 0.02f;  // Subtle side sway
        }
        
        // Update bob time based on movement
        float speedDivisor = sliding ? 15.0f : (crouching ? 2.0f : (sprinting ? 10.0f : 5.0f));
        bobTime += deltaTime * bobSpeed * (horizontalSpeed / speedDivisor);
        
        // Vertical bobbing (sine wave)
        headBobOffset.y = sin(bobTime) * bobIntensity;
        
        // Side-to-side sway (slower sine wave)
        headBobOffset.x = sin(bobTime * 0.5f) * sideIntensity;
        
        // Subtle forward/backward movement
        headBobOffset.z = sin(bobTime * 2.0f) * bobIntensity * 0.3f;
        
        // Additional shake effects
        if (sliding) {
            // Intense sliding shake - chaotic and powerful
            float shakeIntensity = 0.03f;  // Strong shake for sliding
            cameraShakeOffset.x += (rand() / (float)RAND_MAX - 0.5f) * shakeIntensity;
            cameraShakeOffset.y += (rand() / (float)RAND_MAX - 0.5f) * shakeIntensity;
            cameraShakeOffset.z += (rand() / (float)RAND_MAX - 0.5f) * shakeIntensity * 0.8f;
            
            // Strong rhythmic shake for sliding impact
            float rhythmicShake = sin(bobTime * 4.0f) * 0.025f;
            cameraShakeOffset.y += rhythmicShake;
            cameraShakeOffset.x += sin(bobTime * 3.5f) * 0.02f;
            
            // Add sliding-specific low-frequency rumble
            float rumble = sin(bobTime * 0.8f) * 0.01f;
            cameraShakeOffset.z += rumble;
        } else if (sprinting) {
            // Moderate random shake
            float shakeIntensity = 0.015f;  // Reduced shake intensity
            cameraShakeOffset.x += (rand() / (float)RAND_MAX - 0.5f) * shakeIntensity;
            cameraShakeOffset.y += (rand() / (float)RAND_MAX - 0.5f) * shakeIntensity;
            cameraShakeOffset.z += (rand() / (float)RAND_MAX - 0.5f) * shakeIntensity * 0.5f;
            
            // Add subtle rhythmic shake based on bob time
            float rhythmicShake = sin(bobTime * 3.0f) * 0.015f;
            cameraShakeOffset.y += rhythmicShake;
            cameraShakeOffset.x += sin(bobTime * 2.5f) * 0.01f;
        }
    } else {
        // Gradually decay bob time when not moving
        bobTime *= 0.95f;
    }
}

void Player::updateCameraHeight(float deltaTime) {
    const auto& config = GameConfig::getInstance().player;
    
    // Set target camera height based on current state
    if (sliding) {
        targetCameraHeight = config.slideCameraOffset;
    } else if (crouching) {
        targetCameraHeight = config.crouchCameraOffset;
    } else {
        targetCameraHeight = 0.0f; // Normal height
    }
    
    // Smoothly interpolate to target height
    float heightDifference = targetCameraHeight - currentCameraHeight;
    if (std::abs(heightDifference) > 0.01f) {
        currentCameraHeight += heightDifference * cameraHeightTransitionSpeed * deltaTime;
    } else {
        currentCameraHeight = targetCameraHeight;
    }
}

} // namespace silic2