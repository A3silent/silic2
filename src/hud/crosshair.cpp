#include "hud/crosshair.h"
#include "engine/shader.h"
#include <glad/glad.h>

namespace silic2 {

Crosshair::Crosshair() : vao(0), vbo(0) {}

Crosshair::~Crosshair() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
}

void Crosshair::init() {
    shader = std::make_unique<Shader>("res/shaders/crosshair.vert", "res/shaders/crosshair.frag");
    buildMesh();
}

void Crosshair::buildMesh() {
    // Two solid quads forming a + shape, meeting at a single center pixel.
    // arm = half-length of each bar in pixels, t = half-thickness in pixels.
    const float arm = 5.0f;
    const float t   = 1.0f; // 2px thick total (±1px)

    // Horizontal bar and vertical bar, each as two triangles.
    float v[] = {
        // Horizontal bar
        -arm, -t,    arm, -t,    arm,  t,
        -arm, -t,    arm,  t,   -arm,  t,
        // Vertical bar
        -t, -arm,    t, -arm,    t,  arm,
        -t, -arm,    t,  arm,   -t,  arm,
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void Crosshair::render(int screenWidth, int screenHeight) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader->use();
    shader->setVec2("screenSize", static_cast<float>(screenWidth), static_cast<float>(screenHeight));

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 12);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

} // namespace silic2
