#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace silic2 {

enum CameraMovement {
    UP,
    DOWN,
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f,
           float pitch = 0.0f);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(int width, int height) const;

    void processKeyboard(CameraMovement direction, float deltaTime);
    void processMouseMovement(float xpos, float ypos, bool constrainPitch = true);
    void processMouseScroll(float yoffset);
    void update(float deltaTime);

    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }

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

    // Camera options
    float movementSpeed;
    float mouseSensitivity;
    float fov;

    // Mouse state
    bool firstMouse;
    float lastX;
    float lastY;

    void updateCameraVectors();
};

} // namespace silic2