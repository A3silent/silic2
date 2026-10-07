#pragma once

#include "hud/hud_view_model.h"
#include <array>
#include <string_view>

namespace silic2 {

struct HudVertex { glm::vec2 position; glm::vec4 color; };

// CPU geometry is testable without an OpenGL context; one GPU draw consumes it.
class HudBatch {
public:
    std::vector<HudVertex> vertices;
    void rect(float x, float y, float w, float h, const glm::vec4& color);
    void triangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, const glm::vec4& color);
    void line(glm::vec2 a, glm::vec2 b, float width, const glm::vec4& color);
};

class BitmapFontRenderer {
public:
    // Original 5x7 uppercase masks. Unknown characters render a question mark.
    static std::array<uint8_t, 7> glyph(char character);
    static float width(std::string_view text, float pixelSize);
    static void append(HudBatch& batch, std::string_view text, float x, float y,
                       float pixelSize, const glm::vec4& color);
};

void buildHudGeometry(HudBatch& batch, const HudViewModel& model,
                      int logicalWidth, int logicalHeight);

} // namespace silic2
