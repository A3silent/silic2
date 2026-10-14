#include "engine/map_renderer.h"
#include "engine/shader.h"
#include "environment/environment_renderer.h"
#include <algorithm>
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace silic2 {
namespace {
struct CrystalTextureProfile { const char* file; glm::vec3 mean; float worldSize, strength; };
const CrystalTextureProfile crystalProfiles[] = {
    {"res/texture/crystal_combat/metal_deck-64.png",{.27865f,.30123f,.32231f},6.f,.80f},
    {"res/texture/crystal_combat/mineral_ceramic-64.png",{.67220f,.64510f,.59206f},5.f,.55f},
    {"res/texture/crystal_combat/dark_shell-64.png",{.25899f,.20994f,.16303f},8.f,.65f},
    {"res/texture/crystal_combat/crystal_cleavage-64.png",{.29666f,.41174f,.47789f},6.f,.48f}
};
int crystalProfile(const std::string& material) {
    if(material=="combat_foundation" || material=="combat_deck") return 0;
    if(material=="combat_ivory") return 1;
    if(material=="combat_brown") return 2;
    if(material=="combat_crystal") return 3;
    return -1;
}
}


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
    const auto preset=map.getEnvironmentPreset();
    const bool arena = preset==EnvironmentPreset::StarryCrystal || preset==EnvironmentPreset::HollowOrbit;
    if (arena) {
        environment = std::make_unique<EnvironmentRenderer>(deriveEnvironmentScene(map));
        arenaShader = std::make_unique<Shader>("res/shaders/map.vert", environmentShaderPaths(preset).mapFragment);
        GLint linked = GL_FALSE;
        glGetProgramiv(arenaShader->ID, GL_LINK_STATUS, &linked);
        if (!linked) throw std::runtime_error("Arena material shader failed to link");
    }
    if(environment && environment->scene().combatWorld) {
        // Four cache lookups only at map load; failed files leave the original
        // procedural material active, with no texture I/O in a combat frame.
        for(size_t i=0;i<crystalTextures.size();++i)
            crystalTextures[i]=TextureManager::getInstance().loadTexture(crystalProfiles[i].file);
    }
    std::cout << "Loading " << brushes.size() << " brushes for rendering..." << std::endl;
    
    for (const auto& brush : brushes) {
        auto renderable = std::make_unique<RenderableBrush>();
        renderable->color = brush.color;
        renderable->material = brush.material;
        renderable->surfaceType = brush.surfaceType;
        if (!brush.vertices.empty()) {
            renderable->minimum = renderable->maximum = brush.vertices.front();
            for (const auto& vertex : brush.vertices) {
                renderable->minimum = glm::min(renderable->minimum,vertex);
                renderable->maximum = glm::max(renderable->maximum,vertex);
            }
        }
        
        // Load texture
        if (!brush.texture.empty()) {
            renderable->texture = TextureManager::getInstance().loadTexture(brush.texture);
        }
        
        if(environment && environment->scene().combatWorld && brush.texture.empty()) {
            renderable->crystalTextureProfile=crystalProfile(brush.material);
            if(renderable->crystalTextureProfile>=0)
                renderable->texture=crystalTextures[renderable->crystalTextureProfile];
        }
        setupBrushGeometry(brush, *renderable);
        renderableBrushes.push_back(std::move(renderable));
    }
    
    updateLighting();
    
    std::cout << "Map loaded successfully. " << renderableBrushes.size() << " brushes ready for rendering." << std::endl;
    return true;
}

void MapRenderer::clearMap() {
    environment.reset();
    arenaShader.reset();
    combinedLights.clear();
    renderableBrushes.clear();
    crystalTextures.fill(nullptr);
    lightData.clear();
    dynamicLights.clear();
    currentMap = nullptr;
}

void MapRenderer::render(const glm::mat4& view, const glm::mat4& projection) {
    if (!currentMap) {
        std::cout << "No current map to render" << std::endl;
        return;
    }
    
    if (renderableBrushes.empty() && !environment) return;
    
    // std::cout << "Rendering " << renderableBrushes.size() << " brushes" << std::endl;
    
    // Set render state
    if (wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    if (environment) environment->renderSky(view, projection);
    Shader* shader = environment ? arenaShader.get() : mapShader.get();
    shader->use();
    shader->setMat4("view", view);
    shader->setMat4("projection", projection);
    
    // Set world settings
    const auto& worldSettings = currentMap->getWorldSettings();
    shader->setVec3("ambientLight", worldSettings.ambientLight);
    shader->setBool("lightingEnabled", lightingEnabled);
    
    if (environment) {
        environment->setMapUniforms(*shader);
        const auto& obstacles=currentMap->getArenaLayout().obstacles;
        const size_t contactCount=std::min(obstacles.size(),size_t(16));
        shader->setInt("obstacleCount",static_cast<int>(contactCount));
        for(size_t i=0;i<contactCount;++i){
            const auto& box=obstacles[i];
            shader->setVec4("obstacleRects["+std::to_string(i)+"]",box.min.x,box.min.z,box.max.x,box.max.z);
        }
    }

    // Combine static and dynamic lights — cache for other renderers (e.g. enemies)
    combinedLights = lightData;
    if (environment) {
        const float phase = environment->transformation();
        for (auto& light : combinedLights) {
            if(environment->scene().settings.preset==EnvironmentPreset::StarryCrystal) {
                light.color*=glm::mix(glm::vec3(1.f),glm::vec3(1.2f,.7f,1.15f),phase);
                continue;
            }
            // Preset palette conversion only: authored anchors/ranges/intensities
            // stay authoritative; combat lights appended below keep warm cues.
            const float luminance=glm::dot(light.color,glm::vec3(.2126f,.7152f,.0722f));
            light.color=glm::mix(glm::vec3(.91f,.94f,.82f),glm::vec3(.74f,.86f,.67f),phase)*luminance;
        }
        const auto& rifts = environment->scene().rifts;
        const auto& warnings = environment->warnings();
        for (size_t i = 0; i < rifts.size(); ++i) {
            if (rifts[i].active && warnings[i] >= 0.f) {
                const glm::vec3 position(rifts[i].localToWorld * glm::vec4(0,1,0,1));
                const float scale = std::max({rifts[i].scale.x,rifts[i].scale.y,rifts[i].scale.z});
                combinedLights.push_back({position,glm::vec3(1.f,.35f,.06f),1.2f,4.5f*scale});
            }
        }
    }
    combinedLights.insert(combinedLights.end(), dynamicLights.begin(), dynamicLights.end());

    // Set lighting data
    if (lightingEnabled && !combinedLights.empty()) {
        shader->setInt("numLights", std::min(static_cast<int>(combinedLights.size()), 128));

        for (size_t i = 0; i < std::min(combinedLights.size(), size_t(128)); ++i) {
            std::string base = "lights[" + std::to_string(i) + "]";
            shader->setVec3(base + ".position", combinedLights[i].position);
            shader->setVec3(base + ".color", combinedLights[i].color);
            shader->setFloat(base + ".intensity", combinedLights[i].intensity);
            shader->setFloat(base + ".range", combinedLights[i].range);
        }
    } else {
        shader->setInt("numLights", 0);
    }
    
    // Render all brushes
    glm::mat4 model = glm::mat4(1.0f);
    for (size_t i = 0; i < renderableBrushes.size(); ++i) {
        // std::cout << "Rendering brush " << i << " with " << renderableBrushes[i]->indexCount << " indices" << std::endl;
        renderBrush(*renderableBrushes[i], model, *shader);
    }
    
    if (environment) environment->renderDetails(view, projection);

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
    
    std::vector<uint32_t> indices = brush.faces;
    if (environment) {
        vertexData.clear();
        indices.clear();
        for (size_t f = 0; f + 2 < brush.faces.size(); f += 3) {
            const auto a = brush.vertices[brush.faces[f]];
            const auto b = brush.vertices[brush.faces[f+1]];
            const auto c = brush.vertices[brush.faces[f+2]];
            const glm::vec3 cross = glm::cross(b-a, c-a);
            if (glm::dot(cross,cross) < 1e-10f) continue;
            const glm::vec3 normal = glm::normalize(cross);
            for (size_t corner = 0; corner < 3; ++corner) {
                const auto vertexIndex = brush.faces[f+corner];
                const auto p = brush.vertices[vertexIndex];
                const auto uv = vertexIndex < brush.texCoords.size() ? brush.texCoords[vertexIndex] : glm::vec2(p.x,p.z);
                indices.push_back(static_cast<uint32_t>(indices.size()));
                vertexData.insert(vertexData.end(), {p.x,p.y,p.z,normal.x,normal.y,normal.z,uv.x,uv.y});
            }
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
    
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
    
    renderable.indexCount = indices.size();
    
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

void MapRenderer::renderBrush(const RenderableBrush& brush, const glm::mat4& model, Shader& shader) {
    if (brush.VAO == 0) {
        std::cout << "Brush has invalid VAO (0)" << std::endl;
        return;
    }
    
    if (brush.indexCount == 0) {
        std::cout << "Brush has no indices to draw" << std::endl;
        return;
    }
    
    shader.setMat4("model", model);
    shader.setVec3("objectColor", brush.color);
    
    if (environment) {
        shader.setInt("materialKind", brush.surfaceType == SurfaceType::FLOOR ? 1 :
                      brush.material == "arena_living_crystal" ? 2 : 0);
        if (environment->scene().observatory && brush.surfaceType != SurfaceType::FLOOR) {
            int kind = brush.material == "observatory_salt" ? 4 :
                       brush.material == "observatory_ink" ? 5 :
                       brush.material == "observatory_ivory" ? 6 : 3;
            shader.setInt("materialKind",kind);
        }
        if (environment->scene().combatWorld) {
            int kind=brush.material=="combat_crystal"?12:brush.material=="combat_ivory"?13:
                     brush.material=="combat_brown"?14:brush.material=="combat_foundation"?10:11;
            shader.setInt("materialKind",kind);
        }
        shader.setVec3("brushMinimum",brush.minimum);
        shader.setVec3("brushSize",glm::max(brush.maximum-brush.minimum,glm::vec3(.001f)));
    }
    // Bind texture
    const bool profileEnabled=brush.crystalTextureProfile<0 ||
        (crystalTextureMask & (1u<<brush.crystalTextureProfile));
    if (brush.texture && profileEnabled) {
        if(brush.crystalTextureProfile>=0) {
            const auto& profile=crystalProfiles[brush.crystalTextureProfile];
            shader.setVec3("textureMean",profile.mean);
            shader.setFloat("textureWorldScale",1.f/profile.worldSize);
            shader.setFloat("textureStrength",profile.strength);
        }
        shader.setBool("useTexture", true);
        shader.setInt("texture1", 0);
        brush.texture->bind(0);
    } else {
        shader.setBool("useTexture", false);
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

void MapRenderer::setObservationState(float progress, bool complete) {
    if (environment) environment->setObservationState(progress,complete);
}

void MapRenderer::setEnvironmentTime(float seconds) {
    if (environment) environment->setTime(seconds);
}
void MapRenderer::setRiftWarnings(const std::array<float, 4>& progress) {
    if (environment) environment->setRiftWarnings(progress);
}
void MapRenderer::setEnvironmentParticleCount(size_t count) {
    if (environment) environment->setParticleCount(count);
}
float MapRenderer::getEnvironmentTime() const {
    return environment ? environment->time() : 0.f;
}
size_t MapRenderer::getEnvironmentParticleCount() const {
    return environment ? environment->particleCount() : 0;
}

void MapRenderer::resetEnvironment() {
    if (environment) environment->reset();
}

} // namespace silic2