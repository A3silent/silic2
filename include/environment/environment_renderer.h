#pragma once
#include <array>
#include <cstddef>
#include <memory>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "environment/environment_scene.h"

namespace silic2 {
class Shader;
// Visual-only arena presentation. Time is supplied by the simulation: no wall clock,
// collision mutation, file I/O during combat, or gameplay RNG consumption.
class EnvironmentRenderer {
public:
    explicit EnvironmentRenderer(const EnvironmentScene& scene);
    ~EnvironmentRenderer();
    EnvironmentRenderer(const EnvironmentRenderer&) = delete;
    EnvironmentRenderer& operator=(const EnvironmentRenderer&) = delete;
    void reset();
    void setTime(float seconds);
    void setObservationState(float progress, bool complete);
    void setRiftWarnings(const std::array<float, 4>& progress);
    void setParticleCount(std::size_t count);
    float transformation() const;
    float time() const { return seconds_; }
    std::size_t particleCount() const { return particleCount_; }
    const std::array<float, 4>& warnings() const { return warnings_; }
    const EnvironmentScene& scene() const { return scene_; }
    void setMapUniforms(Shader& shader) const;
    void renderSky(const glm::mat4& view, const glm::mat4& projection);
    void renderDetails(const glm::mat4& view, const glm::mat4& projection);
private:
    EnvironmentScene scene_;
    std::unique_ptr<Shader> sky_, crystal_, particles_;
    GLuint emptyVao_ = 0, crystalVao_ = 0, crystalVbo_ = 0;
    GLsizei crystalVertices_ = 0;
    std::size_t particleCount_ = 256;
    float seconds_ = 0;
    float observationProgress_ = 0.f;
    bool observationComplete_ = false;
    void setObservatoryUniforms(Shader& shader) const;
    std::array<float, 4> warnings_{{-1, -1, -1, -1}};
    void buildCrystals();
};
}
