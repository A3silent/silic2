#pragma once
#include <vector>
#include <memory>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "enemy/enemy.h"

namespace silic2 {

class Shader;

class HudRenderer {
public:
    HudRenderer();
    ~HudRenderer();

    bool init();

    void render(int screenW, int screenH,
                float playerHp, float playerMaxHp,
                const std::vector<Enemy>& enemies,
                const glm::mat4& view,
                const glm::mat4& projection);

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    std::unique_ptr<Shader> shader;

    // Draw a filled rectangle. x,y = top-left corner in screen pixels (Y-down).
    void drawRect(float x, float y, float w, float h,
                  const glm::vec4& col, int screenW, int screenH);

    static glm::vec4 hpColor(float ratio);  // green > 60%, yellow > 30%, red otherwise
};

} // namespace silic2
