#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <glad/glad.h>

// Forward declaration
struct GLFWwindow;

namespace silic2 {

class Shader;

class SimpleRenderer {
public:
    SimpleRenderer();
    ~SimpleRenderer();

    void setViewProjectionMatrix(const glm::mat4& view, const glm::mat4& projection);
    void drawTestScene();

private:
    // Test cube
    GLuint cubeVAO, cubeVBO;
    
    // Shaders
    std::unique_ptr<Shader> sceneShader;
    
    // Matrices
    glm::mat4 view;
    glm::mat4 projection;
    
    void initTestCube();
    void initShaders();
};

} // namespace silic2