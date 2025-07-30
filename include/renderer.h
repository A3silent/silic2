#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <glad/glad.h>

// Forward declaration
struct GLFWwindow;

namespace silic2 {

class Shader;

class Renderer {
public:
    Renderer(int renderWidth, int renderHeight);
    ~Renderer();

    void beginFrame();
    void endFrame(GLFWwindow* window, int windowWidth, int windowHeight);
    void setViewProjectionMatrix(const glm::mat4& view, const glm::mat4& projection);
    
    void drawTestScene();

private:
    int renderWidth;
    int renderHeight;
    
    // Framebuffer for low-res rendering
    GLuint framebuffer;
    GLuint colorTexture;
    GLuint depthRenderBuffer;
    
    // Screen quad for displaying framebuffer
    GLuint screenVAO, screenVBO;
    
    // Test cube
    GLuint cubeVAO, cubeVBO;
    
    // Shaders
    std::unique_ptr<Shader> sceneShader;
    std::unique_ptr<Shader> screenShader;
    
    // Matrices
    glm::mat4 view;
    glm::mat4 projection;
    
    void initFramebuffer();
    void initScreenQuad();
    void initTestCube();
    void initShaders();
};

} // namespace silic2