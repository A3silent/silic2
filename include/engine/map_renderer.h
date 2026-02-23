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
    
    // Lighting uniforms
    struct LightData {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        float range;
    };
    std::vector<LightData> lightData;
    std::vector<LightData> dynamicLights;
    
    void initShaders();
    void setupBrushGeometry(const Brush& brush, RenderableBrush& renderable);
    void updateLighting();
    void renderBrush(const RenderableBrush& brush, const glm::mat4& model);
};

} // namespace silic2