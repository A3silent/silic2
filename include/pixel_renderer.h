#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>

namespace silic2 {

class Shader;

class PixelRenderer {
public:
    PixelRenderer();
    ~PixelRenderer();
    
    // Initialize with target resolution (e.g., 320x200)
    bool init(int pixelWidth, int pixelHeight);
    
    // Begin rendering to low-res buffer
    void beginPixelRender();
    
    // End rendering and display to screen
    void endPixelRender(int screenWidth, int screenHeight);
    
    // Get the low-res dimensions
    int getPixelWidth() const { return pixelWidth; }
    int getPixelHeight() const { return pixelHeight; }
    
private:
    int pixelWidth;
    int pixelHeight;
    
    // Framebuffer for low-res rendering
    GLuint framebuffer;
    GLuint colorTexture;
    GLuint depthRenderbuffer;
    
    // Screen quad for displaying framebuffer
    GLuint screenVAO, screenVBO;
    
    // Shader for post-processing
    std::unique_ptr<Shader> pixelShader;
    
    void createFramebuffer();
    void createScreenQuad();
    void createShaders();
};

} // namespace silic2