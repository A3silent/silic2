#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <array>
#include <glad/glad.h>
#include "engine/map.h"
#include "engine/texture.h"

namespace silic2 {

class Shader;
class EnvironmentRenderer;

struct RenderableBrush {
    GLuint VAO, VBO, EBO;
    size_t indexCount;
    glm::vec3 color;
    std::string material;
    SurfaceType surfaceType = SurfaceType::UNKNOWN;
    glm::vec3 minimum{0.f};
    glm::vec3 maximum{0.f};
    std::shared_ptr<Texture> texture;
    int crystalTextureProfile = -1;
    
    RenderableBrush() : VAO(0), VBO(0), EBO(0), indexCount(0) {}
    ~RenderableBrush() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);
    }
};

class MapRenderer {
public:
    // Shared light descriptor used by map, enemy, and any future lit pass
    struct LightData {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        float range;
    };

    MapRenderer();
    ~MapRenderer();

    // Load a map for rendering
    bool loadMap(const Map& map);
    void clearMap();

    // Render the current map
    void render(const glm::mat4& view, const glm::mat4& projection);

    // Arena-only, simulation-clock presentation. Safe no-ops on legacy maps.
    void setEnvironmentTime(float combatSeconds);
    void setObservationState(float progress, bool complete);
    void setRiftWarnings(const std::array<float, 4>& progress);
    void setEnvironmentParticleCount(size_t count);
    void resetEnvironment();
    float getEnvironmentTime() const;
    size_t getEnvironmentParticleCount() const;
    bool hasEnvironment() const { return environment != nullptr; }

    // Add dynamic lights (e.g., from bullets)
    void addDynamicLight(const glm::vec3& position, const glm::vec3& color, float intensity = 1.0f, float range = 10.0f);
    void clearDynamicLights();

    // Returns the fully combined (static + dynamic) light list after the last render() call
    const std::vector<LightData>& getCombinedLights() const { return combinedLights; }

    // Settings
    void setWireframeMode(bool enabled) { wireframeMode = enabled; }
    void setLightingEnabled(bool enabled) { lightingEnabled = enabled; }
    // Optional material-quality/debug switch; bits: deck, ceramic, shell, crystal.
    // Only the explicit Crystal combat architecture consumes this setting.
    void setCrystalTextureMask(unsigned mask) { crystalTextureMask = mask & 15u; }

private:
    std::vector<std::unique_ptr<RenderableBrush>> renderableBrushes;
    std::unique_ptr<Shader> mapShader;
    std::unique_ptr<Shader> arenaShader;
    std::unique_ptr<EnvironmentRenderer> environment;

    // Current map data
    const Map* currentMap = nullptr;

    // Render settings
    bool wireframeMode = false;
    bool lightingEnabled = true;
    unsigned crystalTextureMask = 15u;
    std::array<std::shared_ptr<Texture>, 4> crystalTextures;

    // Lighting data
    std::vector<LightData> lightData;       // static lights from map
    std::vector<LightData> dynamicLights;   // per-frame dynamic lights (bullets, etc.)
    std::vector<LightData> combinedLights;  // cached union built during render()
    
    void initShaders();
    void setupBrushGeometry(const Brush& brush, RenderableBrush& renderable);
    void updateLighting();
    void renderBrush(const RenderableBrush& brush, const glm::mat4& model, Shader& shader);
};

} // namespace silic2