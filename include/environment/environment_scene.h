#pragma once

#include <array>
#include <vector>
#include <glm/glm.hpp>
#include "engine/map.h"

namespace silic2 {

// A derived rendering snapshot, never a second authored map definition. Both the
// solid scene and this snapshot are rebuilt from Map after an editor transaction.
struct EnvironmentRift {
    bool active = false;
    glm::vec3 position{0.f};
    glm::vec3 scale{1.f};
    glm::mat4 localToWorld{1.f};
    glm::mat4 worldToLocal{1.f};
};

struct EnvironmentScene {
    EnvironmentSettings settings;
    bool hasFloor = false;
    glm::vec2 minimumXZ{0.f};
    glm::vec2 maximumXZ{0.f};
    float floorY = 0.f;
    std::array<EnvironmentRift, 4> rifts;
};

EnvironmentScene deriveEnvironmentScene(const Map& map);
// Pure helpers shared by the GL renderer and CPU verification.
// Interleaved position(3), normal(3), tint/semantic rift slot(1).
std::vector<float> buildEnvironmentGeometry(const EnvironmentScene& scene);
float environmentPhase(const EnvironmentSettings& settings, float seconds);
glm::mat4 environmentRiftTransform(const glm::vec3& position,
                                   const glm::vec3& rotationDegrees,
                                   const glm::vec3& scale);

} // namespace silic2
