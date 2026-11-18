#pragma once

#include "engine/fisheye_settings.h"
#include <glm/glm.hpp>

namespace silic2 {

// Camera projection, pixel postprocessing and HUD anchors share one lens per frame.
// Default construction keeps the rectilinear camera used by standalone previews.
struct ViewLens {
    float strength = 0.f;
    glm::vec2 radiusWeights{.5f, .5f};

    static ViewLens forPlayer(float aimBlend, float aspectRatio, const FisheyeSettings& settings = {}) {
        if (!settings.enabled) return {};
        const float aspectSquared = aspectRatio * aspectRatio;
        return {glm::mix(static_cast<float>(settings.hipStrength), .08f, aimBlend),
                glm::vec2(aspectSquared, 1.f) / (aspectSquared + 1.f)};
    }

    float overscanFactor() const { return 1.f + strength; }

    glm::mat4 overscan(glm::mat4 projection) const {
        projection[0][0] /= overscanFactor();
        projection[1][1] /= overscanFactor();
        return projection;
    }

    // Invert pixel.frag's radial sample map along the same ray. With source NDC
    // inside the capture, six Newton steps cover the full strength range.
    glm::vec2 displayNdc(glm::vec2 sourceNdc) const {
        const glm::vec2 ray = sourceNdc * overscanFactor();
        const float cubic = strength * glm::dot(ray * ray, radiusWeights);
        float scale = 1.f;
        for (int i = 0; i < 6; ++i) {
            const float term = cubic * scale * scale;
            scale -= (scale * (1.f + term) - 1.f) / (1.f + 3.f * term);
        }
        return ray * scale;
    }
};

} // namespace silic2
