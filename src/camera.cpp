#include "camera.h"
#include "game_config.h"
#include <GLFW/glfw3.h>

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
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    const auto& config = GameConfig::getInstance().render;
    return glm::perspective(glm::radians(fov), aspectRatio, config.nearPlane, config.farPlane);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio, float customFov) const {
    const auto& config = GameConfig::getInstance().render;
    return glm::perspective(glm::radians(customFov), aspectRatio, config.nearPlane, config.farPlane);
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