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

struct EnvironmentRegion {
    glm::vec2 minimum{0.f}, maximum{0.f};
    int material = 0; // entry, salt court, ink gallery, observation, well
};

struct EnvironmentScene {
    EnvironmentSettings settings;
    bool hasFloor = false;
    glm::vec2 minimumXZ{0.f};
    glm::vec2 maximumXZ{0.f};
    float floorY = 0.f;
    std::array<EnvironmentRift, 4> rifts;
    bool observatory = false;
    bool combatWorld = false;
    glm::vec3 observerEye{0.f}, focalPoint{0.f};
    glm::vec3 landmarkPosition{0.f}, landmarkScale{1.f};
    std::array<float,3> fragmentDepths{{.35f,.65f,1.f}};
    float ringRadius = 2.5f;
    std::vector<EnvironmentRegion> regions;
    std::vector<glm::vec3> guidePoints;
    glm::vec3 exitCenter{0.f};
};

struct EnvironmentShaderPaths {
    const char* mapFragment;
    const char* skyFragment;
    const char* detailFragment;
    const char* particleVertex;
    const char* particleFragment;
};
EnvironmentShaderPaths environmentShaderPaths(EnvironmentPreset preset);

EnvironmentScene deriveEnvironmentScene(const Map& map);
// Pure helpers shared by the GL renderer and CPU verification.
// Interleaved position(3), normal(3), tint/semantic rift slot(1).
std::vector<float> buildEnvironmentGeometry(const EnvironmentScene& scene);
float environmentPhase(const EnvironmentSettings& settings, float seconds);
glm::mat4 environmentRiftTransform(const glm::vec3& position,
                                   const glm::vec3& rotationDegrees,
                                   const glm::vec3& scale);

} // namespace silic2
