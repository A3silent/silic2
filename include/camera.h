#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "game_config.h"

struct GLFWwindow;

namespace silic2 {

class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 5.0f, 5.0f));

    // Movement
    void processKeyboard(GLFWwindow* window, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void processMouseScroll(float yoffset);

    // Update camera vectors
    void update();

    // Getters
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
    glm::mat4 getProjectionMatrix(float aspectRatio, float customFov) const;
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }
    glm::vec3 getUp() const { return up; }
    glm::vec3 getRight() const { return right; }
    float getYaw() const { return yaw; }
    float getPitch() const { return pitch; }
    float getFov() const { return fov; }

    // Setters
    void setPosition(const glm::vec3& pos) { position = pos; }
    void setFov(float newFov);
    // Note: Movement speed and sensitivity are now handled by GameConfig

    // Mouse state management
    void setFirstMouse(bool first) { firstMouse = first; }
    bool isFirstMouse() const { return firstMouse; }
    void setLastMousePos(float x, float y) { lastX = x; lastY = y; }
    float getLastX() const { return lastX; }
    float getLastY() const { return lastY; }

private:
    // Camera attributes
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    // Euler angles
    float yaw;
    float pitch;

    // Camera options (use GameConfig)
    float fov = 45.0f;

    // Mouse state
    bool firstMouse = true;
    float lastX = 640.0f;
    float lastY = 360.0f;

    // Update camera vectors based on updated Euler angles
    void updateCameraVectors();
};

} // namespace silic2