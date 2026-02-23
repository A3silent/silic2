#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace silic2 {

class Shader;
class Map;

class Minimap {
public:
    Minimap();
    ~Minimap();

    void init();

    // Call once after a map loads — prebuilds static wall geometry in world space.
    void setMap(const Map* map);

    // Call every frame after endPixelRender().
    void render(const glm::vec3& playerPos,
                const glm::vec3& playerFront,
                const std::vector<glm::vec3>& enemyPositions,
                int screenW, int screenH);

private:
    std::unique_ptr<Shader> shader;

    // Static wall geometry: world-space XZ quads, uploaded once per map load.
    GLuint mapVAO = 0, mapVBO = 0;
    int    mapVertCount = 0;

    // Dynamic geometry per frame: background + enemies + player arrow.
    GLuint dynVAO = 0, dynVBO = 0;

    void buildMapGeometry(const Map* map);

    // How many world units the minimap radius covers.
    static constexpr float VIEW_RADIUS = 25.0f;
    // Minimap size and screen margin in pixels.
    static constexpr float MARGIN = 10.0f;
    static constexpr float SIZE   = 180.0f;
};

} // namespace silic2
