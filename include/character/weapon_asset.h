#pragma once
#include "engine/mesh_asset.h"
#include <string>
namespace silic2 {
// CPU-only, explicit fitting in canonical model coordinates (+Y up, muzzle -Z).
// Loading never changes gameplay aim or projectile origins.
struct WeaponAsset {
    StaticModelBundle model;
    glm::mat4 fit{1.0f};
    glm::vec3 muzzle{0.0f, 0.0f, -0.425f};
    glm::vec3 leftHandGrip{-0.07f,-0.055f,0.075f};
    glm::vec3 rightHandGrip{0.026f,-0.077f,0.14f};
    // Camera-root local optic anchor. Reticle part is generated, not in the GLB.
    glm::vec3 sightCenter{0.0f,0.095f,0.08f};
    size_t reticlePart = static_cast<size_t>(-1);
    std::vector<size_t> emissiveParts; // bright energy seams, excludes tiny reticle
    bool localBloom = false;
    bool proceduralMaterial = false;
    std::string manifestPath;
};
// Disabled/missing/invalid config yields false and an explanatory status. The
// output remains unchanged on failure; no runtime generation or network access.
bool loadWeaponAsset(const std::string& configPath, WeaponAsset& out, std::string& status);
glm::mat4 firstPersonWeaponRoot(float stepPhase, float speed, bool grounded,
                              float slideBlend, float recoil, float aimBlend = 0.0f,
                              glm::vec3 sightCenter = {0.0f,0.095f,0.08f});
}
