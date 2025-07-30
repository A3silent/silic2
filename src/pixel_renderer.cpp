#include "pixel_renderer.h"
#include "shader.h"
#include <iostream>

namespace silic2 {

PixelRenderer::PixelRenderer() 
    : pixelWidth(320), pixelHeight(200),
      framebuffer(0), colorTexture(0), depthRenderbuffer(0),
      screenVAO(0), screenVBO(0) {
}

PixelRenderer::~PixelRenderer() {
    if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
    if (colorTexture) glDeleteTextures(1, &colorTexture);
    if (depthRenderbuffer) glDeleteRenderbuffers(1, &depthRenderbuffer);
    if (screenVAO) glDeleteVertexArrays(1, &screenVAO);
    if (screenVBO) glDeleteBuffers(1, &screenVBO);
}

bool PixelRenderer::init(int pixelWidth, int pixelHeight) {
    this->pixelWidth = pixelWidth;
    this->pixelHeight = pixelHeight;
    
    try {
        createFramebuffer();
        createScreenQuad();
        createShaders();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize PixelRenderer: " << e.what() << std::endl;
        return false;
    }
}

void PixelRenderer::createFramebuffer() {
    // Create framebuffer
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    
    // Create color texture
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pixelWidth, pixelHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    
    // IMPORTANT: Use nearest neighbor filtering for pixelated look
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
    
    // Create depth renderbuffer
    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, pixelWidth, pixelHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Framebuffer not complete!");
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PixelRenderer::createScreenQuad() {
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &screenVAO);
    glGenBuffers(1, &screenVBO);
    glBindVertexArray(screenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

void PixelRenderer::createShaders() {
    pixelShader = std::make_unique<Shader>("res/shaders/pixel.vert", "res/shaders/pixel.frag");
}

void PixelRenderer::beginPixelRender() {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, pixelWidth, pixelHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PixelRenderer::endPixelRender(int screenWidth, int screenHeight) {
    // Bind default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Disable depth testing for screen quad
    glDisable(GL_DEPTH_TEST);
    
    // Render screen quad with pixelated texture
    pixelShader->use();
    pixelShader->setInt("screenTexture", 0);
    pixelShader->setVec2("resolution", pixelWidth, pixelHeight);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    
    glBindVertexArray(screenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    // Re-enable depth testing
    glEnable(GL_DEPTH_TEST);
}

} // namespace silic2