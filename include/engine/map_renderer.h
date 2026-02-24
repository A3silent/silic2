#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <glad/glad.h>
#include "engine/map.h"
#include "engine/texture.h"

namespace silic2 {

class Shader;

struct RenderableBrush {
    GLuint VAO, VBO, EBO;
    size_t indexCount;
    glm::vec3 color;
    std::string material;
    std::shared_ptr<Texture> texture;
    
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

    // Add dynamic lights (e.g., from bullets)
    void addDynamicLight(const glm::vec3& position, const glm::vec3& color, float intensity = 1.0f, float range = 10.0f);
    void clearDynamicLights();

    // Returns the fully combined (static + dynamic) light list after the last render() call
    const std::vector<LightData>& getCombinedLights() const { return combinedLights; }

    // Settings
    void setWireframeMode(bool enabled) { wireframeMode = enabled; }
    void setLightingEnabled(bool enabled) { lightingEnabled = enabled; }

private:
    std::vector<std::unique_ptr<RenderableBrush>> renderableBrushes;
    std::unique_ptr<Shader> mapShader;

    // Current map data
    const Map* currentMap = nullptr;

    // Render settings
    bool wireframeMode = false;
    bool lightingEnabled = true;

    // Lighting data
    std::vector<LightData> lightData;       // static lights from map
    std::vector<LightData> dynamicLights;   // per-frame dynamic lights (bullets, etc.)
    std::vector<LightData> combinedLights;  // cached union built during render()
    
    void initShaders();
    void setupBrushGeometry(const Brush& brush, RenderableBrush& renderable);
    void updateLighting();
    void renderBrush(const RenderableBrush& brush, const glm::mat4& model);
};

} // namespace silic2