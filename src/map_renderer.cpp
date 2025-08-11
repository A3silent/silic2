#include "map_renderer.h"
#include "shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace silic2 {

MapRenderer::MapRenderer() {
    std::cout << "MapRenderer: Initializing..." << std::endl;
    wireframeMode = false;  // Disable wireframe mode for solid rendering
    try {
        initShaders();
        std::cout << "MapRenderer created successfully with shaders" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize MapRenderer: " << e.what() << std::endl;
        throw;
    }
}

MapRenderer::~MapRenderer() {
    clearMap();
}

bool MapRenderer::loadMap(const Map& map) {
    clearMap();
    currentMap = &map;
    
    const auto& brushes = map.getBrushes();
    std::cout << "Loading " << brushes.size() << " brushes for rendering..." << std::endl;
    
    for (const auto& brush : brushes) {
        auto renderable = std::make_unique<RenderableBrush>();
        renderable->color = brush.color;
        renderable->material = brush.material;
        
        // Load texture
        if (!brush.texture.empty()) {
            renderable->texture = TextureManager::getInstance().loadTexture(brush.texture);
        }
        
        setupBrushGeometry(brush, *renderable);
        renderableBrushes.push_back(std::move(renderable));
    }
    
    updateLighting();
    
    std::cout << "Map loaded successfully. " << renderableBrushes.size() << " brushes ready for rendering." << std::endl;
    return true;
}

void MapRenderer::clearMap() {
    renderableBrushes.clear();
    lightData.clear();
    dynamicLights.clear();
    currentMap = nullptr;
}

void MapRenderer::render(const glm::mat4& view, const glm::mat4& projection) {
    if (!currentMap) {
        std::cout << "No current map to render" << std::endl;
        return;
    }
    
    if (renderableBrushes.empty()) {
        std::cout << "No renderable brushes to draw" << std::endl;
        return;
    }
    
    // std::cout << "Rendering " << renderableBrushes.size() << " brushes" << std::endl;
    
    // Set render state
    if (wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    mapShader->use();
    mapShader->setMat4("view", view);
    mapShader->setMat4("projection", projection);
    
    // Set world settings
    const auto& worldSettings = currentMap->getWorldSettings();
    mapShader->setVec3("ambientLight", worldSettings.ambientLight);
    mapShader->setBool("lightingEnabled", lightingEnabled);
    
    // Combine static and dynamic lights
    std::vector<LightData> allLights = lightData;
    allLights.insert(allLights.end(), dynamicLights.begin(), dynamicLights.end());
    
    // Set lighting data
    if (lightingEnabled && !allLights.empty()) {
        mapShader->setInt("numLights", std::min(static_cast<int>(allLights.size()), 128)); // Max 128 lights
        
        for (size_t i = 0; i < std::min(allLights.size(), size_t(128)); ++i) {
            std::string base = "lights[" + std::to_string(i) + "]";
            mapShader->setVec3(base + ".position", allLights[i].position);
            mapShader->setVec3(base + ".color", allLights[i].color);
            mapShader->setFloat(base + ".intensity", allLights[i].intensity);
            mapShader->setFloat(base + ".range", allLights[i].range);
        }
    } else {
        mapShader->setInt("numLights", 0);
    }
    
    // Render all brushes
    glm::mat4 model = glm::mat4(1.0f);
    for (size_t i = 0; i < renderableBrushes.size(); ++i) {
        // std::cout << "Rendering brush " << i << " with " << renderableBrushes[i]->indexCount << " indices" << std::endl;
        renderBrush(*renderableBrushes[i], model);
    }
    
    // Reset polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void MapRenderer::initShaders() {
    std::cout << "Initializing map shaders..." << std::endl;
    try {
        std::cout << "Loading map vertex and fragment shaders..." << std::endl;
        mapShader = std::make_unique<Shader>("res/shaders/map.vert", "res/shaders/map.frag");
        std::cout << "Map shaders loaded successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load map shaders: " << e.what() << std::endl;
        
        // Fallback to scene shader if map shader doesn't exist
        try {
            std::cout << "Trying fallback scene shaders..." << std::endl;
            mapShader = std::make_unique<Shader>("res/shaders/scene.vert", "res/shaders/scene.frag");
            std::cout << "Using fallback scene shaders for map rendering" << std::endl;
        } catch (const std::exception& e2) {
            std::cerr << "Failed to load fallback shaders: " << e2.what() << std::endl;
            throw;
        }
    }
}

void MapRenderer::setupBrushGeometry(const Brush& brush, RenderableBrush& renderable) {
    if (brush.vertices.empty() || brush.faces.empty()) {
        std::cerr << "Warning: Brush " << brush.id << " has no geometry data" << std::endl;
        return;
    }
    
    // Prepare vertex data (position + normal + texCoord)
    std::vector<float> vertexData;
    for (size_t i = 0; i < brush.vertices.size(); ++i) {
        const auto& vertex = brush.vertices[i];
        
        // Position
        vertexData.push_back(vertex.x);
        vertexData.push_back(vertex.y);
        vertexData.push_back(vertex.z);
        
        // Simple normal calculation (pointing up for now)
        vertexData.push_back(0.0f);
        vertexData.push_back(1.0f);
        vertexData.push_back(0.0f);
        
        // Texture coordinates
        if (i < brush.texCoords.size()) {
            vertexData.push_back(brush.texCoords[i].x);
            vertexData.push_back(brush.texCoords[i].y);
        } else {
            // Default texture coordinates
            vertexData.push_back(0.0f);
            vertexData.push_back(0.0f);
        }
    }
    
    // Generate OpenGL objects
    glGenVertexArrays(1, &renderable.VAO);
    glGenBuffers(1, &renderable.VBO);
    glGenBuffers(1, &renderable.EBO);
    
    glBindVertexArray(renderable.VAO);
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, renderable.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);
    
    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderable.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, brush.faces.size() * sizeof(uint32_t), brush.faces.data(), GL_STATIC_DRAW);
    
    // Set up vertex attributes
    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coordinates (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    renderable.indexCount = brush.faces.size();
    
    std::cout << "Brush " << brush.id << " setup: " << brush.vertices.size() 
              << " vertices, " << brush.faces.size() << " indices" << std::endl;
}

void MapRenderer::updateLighting() {
    lightData.clear();
    
    if (!currentMap) return;
    
    const auto& lights = currentMap->getLights();
    for (const auto& light : lights) {
        LightData data;
        data.position = light.position;
        data.color = light.color;
        data.intensity = light.intensity;
        data.range = light.range;
        lightData.push_back(data);
    }
    
    std::cout << "Updated lighting: " << lightData.size() << " lights" << std::endl;
}

void MapRenderer::renderBrush(const RenderableBrush& brush, const glm::mat4& model) {
    if (brush.VAO == 0) {
        std::cout << "Brush has invalid VAO (0)" << std::endl;
        return;
    }
    
    if (brush.indexCount == 0) {
        std::cout << "Brush has no indices to draw" << std::endl;
        return;
    }
    
    mapShader->setMat4("model", model);
    mapShader->setVec3("objectColor", brush.color);
    
    // Bind texture
    if (brush.texture) {
        mapShader->setBool("useTexture", true);
        mapShader->setInt("texture1", 0);
        brush.texture->bind(0);
    } else {
        mapShader->setBool("useTexture", false);
    }
    
    glBindVertexArray(brush.VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(brush.indexCount), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL error after drawing brush: " << error << std::endl;
    }
}

void MapRenderer::addDynamicLight(const glm::vec3& position, const glm::vec3& color, float intensity, float range) {
    LightData light;
    light.position = position;
    light.color = color;
    light.intensity = intensity;
    light.range = range;
    dynamicLights.push_back(light);
}

void MapRenderer::clearDynamicLights() {
    dynamicLights.clear();
}

} // namespace silic2