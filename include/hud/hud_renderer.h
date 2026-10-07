#pragma once
#include "hud/bitmap_font_renderer.h"
#include <memory>
#include <glad/glad.h>

namespace silic2 {
class Shader;

class HudRenderer {
public:
    HudRenderer();
    ~HudRenderer();
    HudRenderer(const HudRenderer&) = delete;
    HudRenderer& operator=(const HudRenderer&) = delete;
    bool init();
    // Call AFTER scene post-processing, with the destination framebuffer bound.
    // Logical sizes control layout; framebuffer sizes control the GL viewport.
    // No input or time is advanced here. Destroy before the OpenGL context.
    void render(const HudViewModel& model, int logicalWidth, int logicalHeight,
                int framebufferWidth, int framebufferHeight);

private:
    GLuint vao = 0, vbo = 0;
    std::unique_ptr<Shader> shader;
    HudBatch batch;
};
} // namespace silic2
