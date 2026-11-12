#include "engine/map_renderer.h"
#include "engine/shader.h"
#include "engine/mesh_asset.h"
#include <filesystem>
#include <map>
#include <tuple>
#include "environment/environment_renderer.h"
#include <algorithm>
#include <stdexcept>
#include <set>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace silic2 {
namespace {
// The dedicated pass has no persistent GL-state side effects. Its depth contents
// intentionally retain the crystal front surface for later additive world FX.
struct TranslucentState {
    GLboolean colorMask[4],depthMask;GLint depthFunc,program,vao,activeTexture,texture0,polygon[2];
    GLint srcRgb,dstRgb,srcAlpha,dstAlpha,equationRgb,equationAlpha;
    bool depth,blend,cull;
    TranslucentState():depth(glIsEnabled(GL_DEPTH_TEST)),blend(glIsEnabled(GL_BLEND)),cull(glIsEnabled(GL_CULL_FACE)) {
        glGetBooleanv(GL_COLOR_WRITEMASK,colorMask);glGetBooleanv(GL_DEPTH_WRITEMASK,&depthMask);
        glGetIntegerv(GL_DEPTH_FUNC,&depthFunc);glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
        glGetIntegerv(GL_ACTIVE_TEXTURE,&activeTexture);glActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture0);
        glGetIntegerv(GL_POLYGON_MODE,polygon);glGetIntegerv(GL_BLEND_SRC_RGB,&srcRgb);glGetIntegerv(GL_BLEND_DST_RGB,&dstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA,&srcAlpha);glGetIntegerv(GL_BLEND_DST_ALPHA,&dstAlpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB,&equationRgb);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&equationAlpha);
    }
    ~TranslucentState() {
        glColorMask(colorMask[0],colorMask[1],colorMask[2],colorMask[3]);glDepthMask(depthMask);glDepthFunc(depthFunc);
        glBlendFuncSeparate(srcRgb,dstRgb,srcAlpha,dstAlpha);glBlendEquationSeparate(equationRgb,equationAlpha);
        depth?glEnable(GL_DEPTH_TEST):glDisable(GL_DEPTH_TEST);blend?glEnable(GL_BLEND):glDisable(GL_BLEND);cull?glEnable(GL_CULL_FACE):glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK,polygon[0]);glUseProgram(program);glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture0);glActiveTexture(activeTexture);
    }
};
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
    const bool arena = preset==EnvironmentPreset::StarryCrystal || preset==EnvironmentPreset::HollowOrbit || preset==EnvironmentPreset::ForestCrystal;
    if (arena) {
        environment = std::make_unique<EnvironmentRenderer>(deriveEnvironmentScene(map));
        arenaShader = std::make_unique<Shader>(preset==EnvironmentPreset::ForestCrystal ? "res/shaders/forest_map.vert" : "res/shaders/map.vert", environmentShaderPaths(preset).mapFragment);
        GLint linked = GL_FALSE;
        glGetProgramiv(arenaShader->ID, GL_LINK_STATUS, &linked);
        if (!linked) throw std::runtime_error("Arena material shader failed to link");
    }
    if(environment && environment->scene().forestWorld) {
        // A complete neutral sampler prevents feedback from the pixel-render
        // attachment even on drivers that inspect inactive sampler branches.
        forestCrystalShader=std::make_unique<Shader>("res/shaders/map.vert","res/shaders/forest_crystal.frag");
        GLint linked=GL_FALSE;glGetProgramiv(forestCrystalShader->ID,GL_LINK_STATUS,&linked);
        if(!linked)throw std::runtime_error("Forest translucent crystal shader failed to link");
        glGenTextures(1,&forestNeutralTexture);
        glBindTexture(GL_TEXTURE_2D,forestNeutralTexture);
        const unsigned char white[]={255,255,255,255};
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    }
    if(environment && environment->scene().combatWorld) {
        // Four cache lookups only at map load; failed files leave the original
        // procedural material active, with no texture I/O in a combat frame.
        for(size_t i=0;i<crystalTextures.size();++i)
            crystalTextures[i]=TextureManager::getInstance().loadTexture(crystalProfiles[i].file);
    }
    std::cout << "Loading " << brushes.size() << " brushes for rendering..." << std::endl;
    
    // Group only explicit forest pine models: other geometry retains its path.
    // Model source and collision remain untouched; the GPU owns one local mesh.
    std::map<uint32_t,const ModelInstance*> pineModels;
    if(preset==EnvironmentPreset::ForestCrystal)
        for(const auto& model:map.getModels())
            if(model.material=="forest_pine")pineModels.emplace(model.id,&model);
    using TreeKey=std::tuple<std::string,std::string,float,float,float>;
    std::map<TreeKey,TreeBatch*> batches;
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
        const auto pine=pineModels.find(brush.id);
        if(pine!=pineModels.end()) {
            const auto& model=*pine->second;
            const auto path=(std::filesystem::path(map.getAssetBaseDirectory())/model.asset).lexically_normal().string();
            const TreeKey key{path,brush.texture,brush.color.x,brush.color.y,brush.color.z};
            auto found=batches.find(key);
            TreeBatch* batch=nullptr;
            if(found==batches.end()) {
                auto owned=std::make_unique<TreeBatch>();batch=owned.get();
                auto mesh=loadStaticObj(path);Brush local;
                local.id=brush.id;local.material=brush.material;
                local.vertices=std::move(mesh.vertices);local.faces=std::move(mesh.faces);local.texCoords=std::move(mesh.texCoords);
                batch->mesh.color=brush.color;batch->mesh.material=brush.material;
                batch->mesh.texture=renderable->texture;
                setupBrushGeometry(local,batch->mesh);
                batches.emplace(key,batch);treeBatches.push_back(std::move(owned));
            } else batch=found->second;
            batch->instances.push_back({staticTransform(model.position,model.rotation,model.scale),renderable->minimum,renderable->maximum});
            continue;
        }
        setupBrushGeometry(brush, *renderable);
        renderableBrushes.push_back(std::move(renderable));
    }

    for(auto& batch:treeBatches) {
        batch->visibleTransforms.reserve(batch->instances.size());
        glGenBuffers(1,&batch->instanceVBO);
        glBindVertexArray(batch->mesh.VAO);
        glBindBuffer(GL_ARRAY_BUFFER,batch->instanceVBO);
        glBufferData(GL_ARRAY_BUFFER,batch->instances.size()*sizeof(glm::mat4),nullptr,GL_DYNAMIC_DRAW);
        for(GLuint column=0;column<4;++column) {
            glEnableVertexAttribArray(3+column);
            glVertexAttribPointer(3+column,4,GL_FLOAT,GL_FALSE,sizeof(glm::mat4),reinterpret_cast<void*>(column*sizeof(glm::vec4)));
            glVertexAttribDivisor(3+column,1);
        }
        glBindVertexArray(0);
    }
    updateLighting();
    
    std::cout << "Map loaded successfully. " << renderableBrushes.size() << " brushes ready for rendering." << std::endl;
    return true;
}

void MapRenderer::clearMap() {
    environment.reset();
    arenaShader.reset();
    forestCrystalShader.reset();
    if(forestNeutralTexture){glDeleteTextures(1,&forestNeutralTexture);forestNeutralTexture=0;}
    combinedLights.clear();
    renderableBrushes.clear();
    treeBatches.clear();
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
    shader->setBool("instanced",false);
    shader->setMat4("view", view);
    shader->setMat4("projection", projection);
    if(environment && environment->scene().forestWorld)
        shader->setVec3("cameraPosition",glm::vec3(glm::inverse(view)[3]));
    
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
            if(environment->scene().forestWorld) continue;
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
        if(forestCrystalShader && renderableBrushes[i]->material=="forest_spire")continue;
        renderBrush(*renderableBrushes[i], model, *shader);
    }
    
    renderTrees(projection*view,*shader);
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

void MapRenderer::renderTranslucent(const glm::mat4& view,const glm::mat4& projection) {
    if(!forestCrystalShader || !environment)return;
    TranslucentState saved;
    glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    auto& shader=*forestCrystalShader;shader.use();shader.setMat4("view",view);shader.setMat4("projection",projection);
    shader.setVec3("cameraPosition",glm::vec3(glm::inverse(view)[3]));environment->setMapUniforms(shader);
    // Opaque enemies in front already own nearer depth; enemies behind remain
    // in the color buffer and are visible through the single alpha composition.
    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);glDepthMask(GL_TRUE);glDepthFunc(GL_LESS);shader.setBool("depthOnly",true);
    for(const auto& brush:renderableBrushes)if(brush->material=="forest_spire")renderBrush(*brush,glm::mat4(1),shader);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDepthMask(GL_FALSE);glDepthFunc(GL_EQUAL);
    // Premultiplied surface plus bounded independent emission. Background
    // transmission remains (1-alpha); emission is not attenuated a second time.
    glEnable(GL_BLEND);glBlendEquationSeparate(GL_FUNC_ADD,GL_FUNC_ADD);glBlendFuncSeparate(GL_ONE,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);shader.setBool("depthOnly",false);
    for(const auto& brush:renderableBrushes)if(brush->material=="forest_spire")renderBrush(*brush,glm::mat4(1),shader);
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
        std::set<std::array<float,9>> uniqueCrystalFaces;
        for (size_t f = 0; f + 2 < brush.faces.size(); f += 3) {
            const auto a = brush.vertices[brush.faces[f]];
            const auto b = brush.vertices[brush.faces[f+1]];
            const auto c = brush.vertices[brush.faces[f+2]];
            if(forestCrystalShader && brush.material=="forest_spire") {
                // GLB doubleSided exports contain coincident reverse triangles.
                // Collapse only the GPU copy; keep original map/asset/collision intact.
                std::array<std::array<float,3>,3> points{{{{a.x,a.y,a.z}},{{b.x,b.y,b.z}},{{c.x,c.y,c.z}}}};
                std::sort(points.begin(),points.end());std::array<float,9> key{};
                for(size_t v=0;v<3;++v)for(size_t axis=0;axis<3;++axis)key[v*3+axis]=points[v][axis];
                if(!uniqueCrystalFaces.insert(key).second)continue;
            }
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

void MapRenderer::renderBrush(const RenderableBrush& brush, const glm::mat4& model, Shader& shader, GLsizei instances) {
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
            int kind=brush.material=="imported_static"?15:
                     brush.material=="combat_crystal"?12:brush.material=="combat_ivory"?13:
                     brush.material=="combat_brown"?14:brush.material=="combat_foundation"?10:11;
            shader.setInt("materialKind",kind);
        }
        if(environment->scene().forestWorld) {
            const int kind=brush.surfaceType==SurfaceType::FLOOR?20:
                brush.material=="forest_spire"?23:brush.material=="forest_thicket"?22:21;
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
        // Forest ground is untextured; do not leave the previous pixel target
        // bound to a sampler while it is also the current render attachment.
        if(environment && environment->scene().forestWorld) glBindTexture(GL_TEXTURE_2D,forestNeutralTexture);
    }
    
    glBindVertexArray(brush.VAO);
    if(instances>0)glDrawElementsInstanced(GL_TRIANGLES,static_cast<GLsizei>(brush.indexCount),GL_UNSIGNED_INT,nullptr,instances);
    else glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(brush.indexCount), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL error after drawing brush: " << error << std::endl;
    }
}

void MapRenderer::renderTrees(const glm::mat4& viewProjection,Shader& shader) {
    if(treeBatches.empty())return;
    // Conservative AABB-plane test. Do not distance-cull or change tree detail.
    const auto rows=glm::transpose(viewProjection);
    const std::array<glm::vec4,6> planes{rows[3]+rows[0],rows[3]-rows[0],rows[3]+rows[1],rows[3]-rows[1],rows[3]+rows[2],rows[3]-rows[2]};
    shader.setBool("instanced",true);
    for(auto& batch:treeBatches) {
        auto& visible=batch->visibleTransforms;visible.clear();
        for(const auto& instance:batch->instances) {
            bool outside=false;
            for(const auto& plane:planes) {
                glm::vec3 positive;
                for(int axis=0;axis<3;++axis)positive[axis]=plane[axis]>=0?instance.maximum[axis]:instance.minimum[axis];
                if(glm::dot(glm::vec3(plane),positive)+plane.w < -.01f){outside=true;break;}
            }
            if(!outside)visible.push_back(instance.transform);
        }
        if(visible.empty())continue;
        glBindBuffer(GL_ARRAY_BUFFER,batch->instanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER,0,visible.size()*sizeof(glm::mat4),visible.data());
        renderBrush(batch->mesh,glm::mat4(1),shader,static_cast<GLsizei>(visible.size()));
    }
    shader.setBool("instanced",false);
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
