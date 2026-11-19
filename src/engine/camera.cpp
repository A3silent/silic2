#include "engine/camera.h"
#include "engine/game_config.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <algorithm>

namespace silic2 {

Camera::Camera(glm::vec3 position)
    : position(position) {
    const auto& config = GameConfig::getInstance().camera;
    worldUp = config.worldUp;
    yaw = config.yaw;
    pitch = config.pitch;
    front = glm::vec3(0.0f, -0.5f, -0.5f);
    updateCameraVectors();
}

void Camera::setDirection(const glm::vec3& direction) {
    if (glm::length(direction) < 0.0001f) return;
    const auto d = glm::normalize(direction);
    yaw = glm::degrees(std::atan2(d.z, d.x));
    pitch = glm::degrees(std::asin(glm::clamp(d.y, -1.0f, 1.0f)));
    updateCameraVectors();
}
void Camera::resetOrientation() {
    yaw = GameConfig::getInstance().camera.yaw;
    pitch = GameConfig::getInstance().camera.pitch;
    firstMouse = true;
    movementRoll = movementRollVelocity = 0;
    hasMovementSample = false;
    updateCameraVectors();
}

void Camera::processKeyboard(GLFWwindow* window, float deltaTime) {
    const auto& config = GameConfig::getInstance().player;
    float velocity = config.moveSpeed * deltaTime;
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        position += velocity * front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        position -= velocity * front;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        position -= glm::normalize(glm::cross(front, up)) * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        position += glm::normalize(glm::cross(front, up)) * velocity;
    
    // Vertical movement
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        position += velocity * up;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        position -= velocity * up;
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    yaw += xoffset;
    pitch += yoffset;

    // Make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch) {
        const auto& config = GameConfig::getInstance().camera;
        if (pitch > config.maxPitch)
            pitch = config.maxPitch;
        if (pitch < config.minPitch)
            pitch = config.minPitch;
    }

    // Update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset) {
    fov -= yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 90.0f)
        fov = 90.0f;
}

void Camera::update() {
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    const float angle = glm::radians(movementRoll);
    // Positive roll tilts the camera's head/up toward its logical right.
    const auto viewUp = up * std::cos(angle) + right * std::sin(angle);
    return glm::lookAt(position, position + front, viewUp);
}

void Camera::syncMovementLeanPosition(const glm::vec3& feet) {
    previousFeet = feet;
    hasMovementSample = true;
}
void Camera::resetMovementLean(const glm::vec3& feet) {
    movementRoll = movementRollVelocity = 0;
    syncMovementLeanPosition(feet);
}
void Camera::updateMovementLean(const glm::vec3& feet, float dt, bool rollStep) {
    if (!std::isfinite(feet.x) || !std::isfinite(feet.y) || !std::isfinite(feet.z)) return;
    if (rollStep) {
        movementRollVelocity = 0;
        syncMovementLeanPosition(feet);
        return;
    }
    if (!(dt > 0) || !std::isfinite(dt)) return;
    if (!hasMovementSample) { resetMovementLean(feet); return; }
    const auto displacement = feet - previousFeet;
    syncMovementLeanPosition(feet);
    // Teleports and suspended-clock gaps are not physical lateral acceleration.
    if (dt > .25f || glm::length(displacement) > std::max(2.f, dt * 60.f)) {
        resetMovementLean(feet); return;
    }
    const auto& config = GameConfig::getInstance().camera;
    if (!(config.leanMaxDegrees > 0) || !std::isfinite(config.leanMaxDegrees)) {
        resetMovementLean(feet); return;
    }
    const float maximum = glm::clamp(config.leanMaxDegrees, 0.f, 10.f);
    const float speed = std::isfinite(config.leanReferenceSpeed) ? std::max(.01f,config.leanReferenceSpeed) : 5.f;
    const float omega = std::isfinite(config.leanFrequency) ? glm::clamp(config.leanFrequency,1.f,60.f) : 18.f;
    const float damping = std::isfinite(config.leanDamping) ? glm::clamp(config.leanDamping,.05f,.99f) : .55f;
    const glm::vec3 horizontal(displacement.x,0,displacement.z);
    const float target = maximum * glm::clamp(glm::dot(horizontal,right)/(dt*speed),-1.f,1.f);
    // Exact constant-target damped spring solution: stable across frame intervals.
    const float a=damping*omega, b=omega*std::sqrt(1-damping*damping);
    const float decay=std::exp(-a*dt), c=std::cos(b*dt), s=std::sin(b*dt);
    const float error=movementRoll-target, velocity=movementRollVelocity;
    movementRoll=target+decay*(error*c+(velocity+a*error)/b*s);
    movementRollVelocity=decay*(velocity*c-(a*velocity+omega*omega*error)/b*s);
    const float bound=maximum*1.5f;
    if(std::abs(movementRoll)>bound){movementRoll=glm::clamp(movementRoll,-bound,bound);movementRollVelocity=0;}
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    const auto& config = GameConfig::getInstance().render;
    return glm::perspective(glm::radians(fov), aspectRatio, config.nearPlane, config.farPlane);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio, float customFov, float mapFarPlane) const {
    const auto& config = GameConfig::getInstance().render;
    const float farPlane = std::isfinite(mapFarPlane) && mapFarPlane > config.nearPlane
        ? mapFarPlane : config.farPlane;
    return glm::perspective(glm::radians(customFov), aspectRatio, config.nearPlane, farPlane);
}

void Camera::setFov(float newFov) {
    fov = newFov;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 90.0f)
        fov = 90.0f;
}

void Camera::updateCameraVectors() {
    // Calculate the new Front vector
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);
    
    // Also re-calculate the Right and Up vector
    right = glm::normalize(glm::cross(front, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    up = glm::normalize(glm::cross(right, front));
}

} // namespace silic2
