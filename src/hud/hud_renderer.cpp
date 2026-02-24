#include "hud/hud_renderer.h"
#include "engine/shader.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>

namespace silic2 {

HudRenderer::HudRenderer() = default;

HudRenderer::~HudRenderer() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
}

bool HudRenderer::init() {
    shader = std::make_unique<Shader>("res/shaders/healthbar.vert", "res/shaders/healthbar.frag");

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // 6 vertices × 2 floats — updated each draw call
    glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    return true;
}

void HudRenderer::drawRect(float x, float y, float w, float h,
                           const glm::vec4& col, int screenW, int screenH) {
    float vertices[6][2] = {
        {x,     y    }, {x + w, y    }, {x + w, y + h},
        {x,     y    }, {x + w, y + h}, {x,     y + h}
    };
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    shader->setVec2("screenSize", static_cast<float>(screenW), static_cast<float>(screenH));
    shader->setVec4("color", col.r, col.g, col.b, col.a);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

glm::vec4 HudRenderer::hpColor(float ratio) {
    if (ratio > 0.6f) return glm::vec4(0.1f, 0.9f, 0.1f, 1.0f);  // green
    if (ratio > 0.3f) return glm::vec4(0.9f, 0.85f, 0.1f, 1.0f); // yellow
    return glm::vec4(0.9f, 0.15f, 0.1f, 1.0f);                    // red
}

void HudRenderer::render(int screenW, int screenH,
                         float playerHp, float playerMaxHp,
                         const std::vector<Enemy>& enemies,
                         const glm::mat4& view,
                         const glm::mat4& projection) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader->use();
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // --- Player health bar (bottom-left) ---
    const float BAR_W  = 160.f;
    const float BAR_H  = 12.f;
    const float MARGIN = 16.f;

    float ratio  = (playerMaxHp > 0.f) ? (playerHp / playerMaxHp) : 0.f;
    float fillW  = BAR_W * ratio;
    float x      = MARGIN;
    float y      = static_cast<float>(screenH) - MARGIN - BAR_H;

    // Background
    drawRect(x, y, BAR_W, BAR_H, glm::vec4(0.1f, 0.1f, 0.1f, 0.85f), screenW, screenH);
    // Fill
    if (fillW > 0.f)
        drawRect(x, y, fillW, BAR_H, hpColor(ratio), screenW, screenH);

    // --- Enemy health bars (screen-projected) ---
    const float EW = 40.f;
    const float EH = 5.f;

    for (const Enemy& e : enemies) {
        if (e.isDead()) continue;

        glm::vec4 clip = projection * view *
                         glm::vec4(e.getPosition() + glm::vec3(0.f, Enemy::BOX_HEIGHT + 0.3f, 0.f), 1.0f);

        if (clip.w <= 0.0f) continue;  // behind camera

        float ndcX = clip.x / clip.w;
        float ndcY = clip.y / clip.w;
        if (std::abs(ndcX) > 1.2f || std::abs(ndcY) > 1.2f) continue;  // off-screen

        float sx = (ndcX + 1.0f) * 0.5f * static_cast<float>(screenW);
        float sy = (1.0f - ndcY) * 0.5f * static_cast<float>(screenH);

        float ex        = sx - EW * 0.5f;
        float enemyRatio = static_cast<float>(e.getHp()) / static_cast<float>(e.getMaxHp());

        // Background
        drawRect(ex, sy, EW, EH, glm::vec4(0.1f, 0.1f, 0.1f, 0.85f), screenW, screenH);
        // Fill (always red for enemies)
        float enemyFill = EW * enemyRatio;
        if (enemyFill > 0.f)
            drawRect(ex, sy, enemyFill, EH, glm::vec4(0.9f, 0.15f, 0.1f, 1.0f), screenW, screenH);
    }

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

} // namespace silic2
