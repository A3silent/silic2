#include "hud/minimap.h"
#include "engine/shader.h"
#include "engine/map.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace silic2 {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------


static void makeVAO(GLuint& vao, GLuint& vbo) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// Minimap
// ---------------------------------------------------------------------------

Minimap::Minimap() = default;

Minimap::~Minimap() {
    if (mapVAO) { glDeleteVertexArrays(1, &mapVAO); glDeleteBuffers(1, &mapVBO); }
    if (dynVAO) { glDeleteVertexArrays(1, &dynVAO); glDeleteBuffers(1, &dynVBO); }
}

void Minimap::init() {
    shader = std::make_unique<Shader>("res/shaders/minimap.vert", "res/shaders/minimap.frag");
    makeVAO(mapVAO, mapVBO);
    makeVAO(dynVAO, dynVBO);
}

void Minimap::setMap(const Map* map) {
    if (map) buildMapGeometry(map);
}

void Minimap::buildMapGeometry(const Map* map) {
    // Store world-space XZ footprints of all non-floor brushes.
    // The vertex shader handles coordinate transform each frame.
    std::vector<float> verts;

    for (const auto& brush : map->getBrushes()) {
        if (brush.surfaceType == SurfaceType::FLOOR) continue;

        float xMin =  std::numeric_limits<float>::max();
        float xMax =  std::numeric_limits<float>::lowest();
        float zMin =  std::numeric_limits<float>::max();
        float zMax =  std::numeric_limits<float>::lowest();

        for (const auto& v : brush.vertices) {
            xMin = std::min(xMin, v.x); xMax = std::max(xMax, v.x);
            zMin = std::min(zMin, v.z); zMax = std::max(zMax, v.z);
        }

        if (xMin >= xMax || zMin >= zMax) continue;

        verts.insert(verts.end(), { xMin, zMin,  xMax, zMin,  xMax, zMax });
        verts.insert(verts.end(), { xMin, zMin,  xMax, zMax,  xMin, zMax });
    }

    mapVertCount = static_cast<int>(verts.size() / 2);

    glBindVertexArray(mapVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mapVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void Minimap::render(const glm::vec3& playerPos,
                     const glm::vec3& playerFront,
                     const std::vector<glm::vec3>& enemyPositions,
                     int screenW, int screenH) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Minimap screen rect, y measured from screen bottom-left.
    float rx = static_cast<float>(screenW) - MARGIN - SIZE;
    float ry = static_cast<float>(screenH) - MARGIN - SIZE;

    // yaw: angle such that rotation by yaw maps player forward → minimap +Y.
    float yaw = atan2f(playerFront.x, playerFront.z);

    float px = playerPos.x, pz = playerPos.z;

    shader->use();
    shader->setVec2("playerPos",   px, pz);
    shader->setFloat("playerYaw",  yaw);
    shader->setFloat("viewRadius", VIEW_RADIUS);
    shader->setVec4("minimapRect", rx, ry, SIZE, SIZE);
    shader->setVec2("screenSize",  static_cast<float>(screenW), static_cast<float>(screenH));

    // ------------------------------------------------------------------
    // Build dynamic vertex buffer: [background | enemies | player arrow]
    // ------------------------------------------------------------------
    std::vector<float> dyn;

    // mmToWorld: converts minimap normalised coords back to world XZ so the
    // vertex shader can round-trip them to the intended on-screen position.
    float c = cosf(yaw), s = sinf(yaw);
    auto mmToWorld = [&](float mx, float mz) -> glm::vec2 {
        float relX = ( mx * c + mz * s) * VIEW_RADIUS;
        float relZ = (-mx * s + mz * c) * VIEW_RADIUS;
        return { px + relX, pz + relZ };
    };

    // Background: a large quad that exceeds the view radius in all directions.
    // The fragment shader clips it to a circle.
    float ext = VIEW_RADIUS * 1.5f;
    dyn.insert(dyn.end(), {
        px-ext, pz-ext,   px+ext, pz-ext,   px+ext, pz+ext,
        px-ext, pz-ext,   px+ext, pz+ext,   px-ext, pz+ext,
    });
    int bgEnd = static_cast<int>(dyn.size() / 2);

    // Enemy circles — triangle fan in mmNorm space so they stay round.
    static constexpr int   CIRCLE_SEG  = 20;
    static constexpr float CIRCLE_R    = 0.045f; // radius in mmNorm [0,1] space
    static constexpr float TWO_PI      = 6.28318530718f;
    for (const auto& ep : enemyPositions) {
        // Enemy position in mmNorm space
        float relX = ep.x - px, relZ = ep.z - pz;
        float emx = (relX * c - relZ * s) / VIEW_RADIUS;
        float emz = (relX * s + relZ * c) / VIEW_RADIUS;

        glm::vec2 center = mmToWorld(emx, emz);
        for (int i = 0; i < CIRCLE_SEG; ++i) {
            float a0 = (i    ) * TWO_PI / CIRCLE_SEG;
            float a1 = (i + 1) * TWO_PI / CIRCLE_SEG;
            glm::vec2 p0 = mmToWorld(emx + CIRCLE_R * cosf(a0), emz + CIRCLE_R * sinf(a0));
            glm::vec2 p1 = mmToWorld(emx + CIRCLE_R * cosf(a1), emz + CIRCLE_R * sinf(a1));
            dyn.insert(dyn.end(), { center.x, center.y, p0.x, p0.y, p1.x, p1.y });
        }
    }
    int enemyEnd = static_cast<int>(dyn.size() / 2);

    // Arrow: tip at (0, 0.10), base corners at (±0.06, -0.05) in mmNorm space.
    glm::vec2 tip   = mmToWorld( 0.00f,  0.10f);
    glm::vec2 baseL = mmToWorld(-0.06f, -0.05f);
    glm::vec2 baseR = mmToWorld( 0.06f, -0.05f);
    dyn.insert(dyn.end(), { tip.x, tip.y, baseL.x, baseL.y, baseR.x, baseR.y });
    int arrowEnd = static_cast<int>(dyn.size() / 2);

    // Upload
    glBindVertexArray(dynVAO);
    glBindBuffer(GL_ARRAY_BUFFER, dynVBO);
    glBufferData(GL_ARRAY_BUFFER, dyn.size() * sizeof(float), dyn.data(), GL_DYNAMIC_DRAW);

    // ------------------------------------------------------------------
    // Draw passes
    // ------------------------------------------------------------------

    // 1. Dark background circle
    shader->setFloat("innerRadius", 0.0f);
    shader->setVec4("color", 0.04f, 0.04f, 0.06f, 0.88f);
    glDrawArrays(GL_TRIANGLES, 0, bgEnd);

    // 2. Map wall geometry
    if (mapVertCount > 0) {
        shader->setVec4("color", 0.05f, 0.10f, 0.28f, 1.0f);
        glBindVertexArray(mapVAO);
        glDrawArrays(GL_TRIANGLES, 0, mapVertCount);
        glBindVertexArray(dynVAO);
    }

    // 3. Enemy dots (red)
    if (enemyEnd > bgEnd) {
        shader->setVec4("color", 1.0f, 0.20f, 0.20f, 1.0f);
        glDrawArrays(GL_TRIANGLES, bgEnd, enemyEnd - bgEnd);
    }

    // 4. Player arrow (green)
    shader->setVec4("color", 0.10f, 1.0f, 0.10f, 1.0f);
    glDrawArrays(GL_TRIANGLES, enemyEnd, arrowEnd - enemyEnd);

    // 5. Border ring on top of everything
    shader->setFloat("innerRadius", 0.93f);
    shader->setVec4("color", 0.05f, 0.10f, 0.28f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, bgEnd);  // reuse background quad

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

} // namespace silic2
