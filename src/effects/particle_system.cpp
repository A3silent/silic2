#include "effects/particle_system.h"
#include "engine/shader.h"
#include <algorithm>
#include <random>
#include <iostream>

namespace silic2 {

// Random number generator for particle effects
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
static std::uniform_real_distribution<float> dis01(0.0f, 1.0f);

float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(gen);
}

glm::vec3 randomVec3(float min, float max) {
    return glm::vec3(
        randomFloat(min, max),
        randomFloat(min, max),
        randomFloat(min, max)
    );
}

//building a lookup table for fade values for better runtime
const std::vector<float>& ParticleSystem::getFadeLUT(float fadeRatio) {
    // Check if LUT for this fadeRatio exists
    auto it = fadeLUTCache.find(fadeRatio);
    if (it != fadeLUTCache.end()) {
        return it->second;
    }

    // Build new LUT
    std::vector<float> newLUT(fadeOutSmoothness + 1);
    for (int i = 0; i <= fadeOutSmoothness; ++i) {
        float r = static_cast<float>(i) / fadeOutSmoothness;
        newLUT[i] = std::pow(r, fadeRatio);
    }

    // Store in cache and return
    fadeLUTCache[fadeRatio] = std::move(newLUT);
    return fadeLUTCache[fadeRatio];
}

// ParticleSystem Implementation
ParticleSystem::ParticleSystem(size_t maxParticles)
    : maxParticleCount(maxParticles), nextDeadParticle(0), 
      VAO(0), VBO(0), boxVAO(0), boxVBO(0), instanceVBO(0) {
    particles.resize(maxParticleCount);
    // Delay initialization until first render call
}

ParticleSystem::~ParticleSystem() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (boxVAO) glDeleteVertexArrays(1, &boxVAO);
    if (boxVBO) glDeleteBuffers(1, &boxVBO);
    if (instanceVBO) glDeleteBuffers(1, &instanceVBO);
}

void ParticleSystem::initRenderingResources() {
    std::cout << "Initializing particle system rendering resources..." << std::endl;
    
    // Create shaders
    try {
        particleShader = std::make_unique<Shader>("res/shaders/particle.vert", "res/shaders/particle.frag");
        std::cout << "Particle shaders loaded successfully" << std::endl;
        
        // Load 3D box shader
        boxShader = std::make_unique<Shader>("res/shaders/particle_box.vert", "res/shaders/particle_box.frag");
        std::cout << "Particle box shaders loaded successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load particle shaders: " << e.what() << std::endl;
        // Don't return, continue without rendering
        particleShader = nullptr;
        boxShader = nullptr;
        return;
    }
    
    // Generate OpenGL objects for point sprites
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    if (VAO == 0 || VBO == 0) {
        std::cerr << "Failed to generate OpenGL objects for particle system" << std::endl;
        return;
    }
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    // Vertex layout: position (3) + color (3) + size (1) = 7 floats per particle
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));

    
    glBindVertexArray(0);
    
    // Setup 3D box mesh
    setupBoxMesh();
    
    std::cout << "Particle system initialized with " << maxParticleCount << " max particles" << std::endl;
}

void ParticleSystem::setupBoxMesh() {
    // Create a shorter box mesh for particles
    // Particle size: 0.02 x 0.02 x 0.05 (much shorter, scaled by instanceSize later)
    float boxVertices[] = {
        // Front face
        -0.01f, -0.01f,  0.025f,  0.0f,  0.0f,  1.0f,
         0.01f, -0.01f,  0.025f,  0.0f,  0.0f,  1.0f,
         0.01f,  0.01f,  0.025f,  0.0f,  0.0f,  1.0f,
         0.01f,  0.01f,  0.025f,  0.0f,  0.0f,  1.0f,
        -0.01f,  0.01f,  0.025f,  0.0f,  0.0f,  1.0f,
        -0.01f, -0.01f,  0.025f,  0.0f,  0.0f,  1.0f,
        
        // Back face
        -0.01f, -0.01f, -0.025f,  0.0f,  0.0f, -1.0f,
         0.01f, -0.01f, -0.025f,  0.0f,  0.0f, -1.0f,
         0.01f,  0.01f, -0.025f,  0.0f,  0.0f, -1.0f,
         0.01f,  0.01f, -0.025f,  0.0f,  0.0f, -1.0f,
        -0.01f,  0.01f, -0.025f,  0.0f,  0.0f, -1.0f,
        -0.01f, -0.01f, -0.025f,  0.0f,  0.0f, -1.0f,
        
        // Left face
        -0.01f,  0.01f,  0.025f, -1.0f,  0.0f,  0.0f,
        -0.01f,  0.01f, -0.025f, -1.0f,  0.0f,  0.0f,
        -0.01f, -0.01f, -0.025f, -1.0f,  0.0f,  0.0f,
        -0.01f, -0.01f, -0.025f, -1.0f,  0.0f,  0.0f,
        -0.01f, -0.01f,  0.025f, -1.0f,  0.0f,  0.0f,
        -0.01f,  0.01f,  0.025f, -1.0f,  0.0f,  0.0f,
        
        // Right face
         0.01f,  0.01f,  0.025f,  1.0f,  0.0f,  0.0f,
         0.01f,  0.01f, -0.025f,  1.0f,  0.0f,  0.0f,
         0.01f, -0.01f, -0.025f,  1.0f,  0.0f,  0.0f,
         0.01f, -0.01f, -0.025f,  1.0f,  0.0f,  0.0f,
         0.01f, -0.01f,  0.025f,  1.0f,  0.0f,  0.0f,
         0.01f,  0.01f,  0.025f,  1.0f,  0.0f,  0.0f,
        
        // Top face
        -0.01f,  0.01f, -0.025f,  0.0f,  1.0f,  0.0f,
         0.01f,  0.01f, -0.025f,  0.0f,  1.0f,  0.0f,
         0.01f,  0.01f,  0.025f,  0.0f,  1.0f,  0.0f,
         0.01f,  0.01f,  0.025f,  0.0f,  1.0f,  0.0f,
        -0.01f,  0.01f,  0.025f,  0.0f,  1.0f,  0.0f,
        -0.01f,  0.01f, -0.025f,  0.0f,  1.0f,  0.0f,
        
        // Bottom face
        -0.01f, -0.01f, -0.025f,  0.0f, -1.0f,  0.0f,
         0.01f, -0.01f, -0.025f,  0.0f, -1.0f,  0.0f,
         0.01f, -0.01f,  0.025f,  0.0f, -1.0f,  0.0f,
         0.01f, -0.01f,  0.025f,  0.0f, -1.0f,  0.0f,
        -0.01f, -0.01f,  0.025f,  0.0f, -1.0f,  0.0f,
        -0.01f, -0.01f, -0.025f,  0.0f, -1.0f,  0.0f
    };
    
    // Generate VAO and VBO for box mesh
    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);
    glGenBuffers(1, &instanceVBO);
    
    glBindVertexArray(boxVAO);
    
    // Box mesh data
    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxVertices), boxVertices, GL_STATIC_DRAW);
    
    // Vertex positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Vertex normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Instance data buffer
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    
    // Instance position
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    
    // Instance color
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);
    
    // Instance size
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);
    
    // Instance velocity (for rotation)
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);
    
    glBindVertexArray(0);
}

void ParticleSystem::update(float deltaTime) {
    
    for (auto& particle : particles) {
        if (!particle.isAlive()) continue;
        
        // Update physics
        particle.pPosition += particle.pVelocity * deltaTime;
        particle.pVelocity.y += defaultGravity * particle.pGravity * deltaTime;
        particle.pVelocity += windForce * deltaTime;
        
        // Update life
        particle.pLife -= deltaTime;
        
        
        // Fade out color based on life if enabled
        if (fadeOutEnabled && particle.pMaxLife > 0.0f) {
            float invMaxLife = 1.0f / particle.pMaxLife;
            float lifeRatio  = particle.pLife * invMaxLife;
            float nextRatio  = (particle.pLife - deltaTime) * invMaxLife;

            float fadeRatio = particle.pFadeRatio;

            // Build LUT if fadeRatio changed
            if (fadeRatio != lastFadeRatio || fadeLUTCache.empty()) {
                getFadeLUT(fadeRatio);
                lastFadeRatio = fadeRatio;
            }
            auto fadeLUTIt = fadeLUTCache.find(fadeRatio);
            const auto& fadeLUT = fadeLUTIt->second;

            // Clamp & lookup
            int idxCurr = std::max(0, std::min(fadeOutSmoothness, int(lifeRatio  * fadeOutSmoothness)));
            int idxNext = std::max(0, std::min(fadeOutSmoothness, int(nextRatio * fadeOutSmoothness)));

            particle.pColor *= fadeLUT[idxNext] / fadeLUT[idxCurr];
            
        }
            
    }
}

void ParticleSystem::render(const glm::mat4& view, const glm::mat4& projection) {
    // Initialize on first render call
    if (!particleShader && VAO == 0 && VBO == 0) {
        initRenderingResources();
    }
    
    if (use3DBoxes && boxShader && boxVAO != 0) {
        // Render using 3D boxes
        updateInstanceBuffer();
        
        if (instanceData.empty()) return;
        
        // Calculate view position for rim lighting
        glm::mat4 invView = glm::inverse(view);
        glm::vec3 viewPos = glm::vec3(invView[3]);
        
        boxShader->use();
        boxShader->setMat4("view", view);
        boxShader->setMat4("projection", projection);
        boxShader->setVec3("viewPos", viewPos);
        
        // Update instance buffer
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_DYNAMIC_DRAW);
        
        // Enable blending for glowing effect
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending
        
        // Render instanced boxes
        glBindVertexArray(boxVAO);
        size_t instanceCount = instanceData.size() / 10;  // 10 floats per instance
        glDrawArraysInstanced(GL_TRIANGLES, 0, 36, static_cast<GLsizei>(instanceCount));
        
        glBindVertexArray(0);
        glDisable(GL_BLEND);
        
    } else if (particleShader && VAO != 0) {
        // Fallback to point sprites
        updateVertexBuffer();
        
        if (vertexData.empty()) return;
        
        particleShader->use();
        particleShader->setMat4("view", view);
        particleShader->setMat4("projection", projection);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);
        
        // Enable blending for particles
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Disable depth writing but keep depth testing
        glDepthMask(GL_FALSE);
        
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(vertexData.size() / 7));
        
        // Restore depth writing
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        
        glBindVertexArray(0);
    }
}

void ParticleSystem::emit(const glm::vec3& position, const glm::vec3& velocity, 
                         const glm::vec3& color, float life, float size, float pgravity, float fadeRatio) {
    size_t deadIndex = findDeadParticle();
    if (deadIndex >= particles.size()) return;
    
    Particle& p = particles[deadIndex];
    p.pPosition = position;
    p.pVelocity = velocity;
    p.pColor = color;
    p.pLife = life;
    p.pMaxLife = life;
    p.pSize = size;
    p.pGravity = pgravity;
    p.pFadeRatio = fadeRatio;
}

void ParticleSystem::emitBurst(const glm::vec3& position, int count, 
                              const glm::vec3& baseVelocity, const glm::vec3& velocityVariation,
                              const glm::vec3& color, float life, float size, float pgravity, float fadeRatio) {
    for (int i = 0; i < count; ++i) {
        glm::vec3 vel = baseVelocity + glm::vec3(
            randomFloat(-velocityVariation.x, velocityVariation.x),
            randomFloat(-velocityVariation.y, velocityVariation.y),
            randomFloat(-velocityVariation.z, velocityVariation.z)
        );
        
        glm::vec3 particleColor = color;
        // Add some color variation
        particleColor.r += randomFloat(-0.1f, 0.1f);
        particleColor.g += randomFloat(-0.1f, 0.1f);
        particleColor.b += randomFloat(-0.1f, 0.1f);
        particleColor = glm::clamp(particleColor, 0.0f, 1.0f);
        
        emit(position, vel, particleColor, life, size, pgravity, fadeRatio);
    }
}

size_t ParticleSystem::getActiveParticles() const {
    size_t count = 0;
    for (const auto& particle : particles) {
        if (particle.isAlive()) count++;
    }
    return count;
}

void ParticleSystem::updateVertexBuffer() {
    vertexData.clear();
    
    for (const auto& particle : particles) {
        if (!particle.isAlive()) continue;
        
        // Position
        vertexData.push_back(particle.pPosition.x);
        vertexData.push_back(particle.pPosition.y);
        vertexData.push_back(particle.pPosition.z);
        
        // Color
        vertexData.push_back(particle.pColor.r);
        vertexData.push_back(particle.pColor.g);
        vertexData.push_back(particle.pColor.b);
        
        // Size
        vertexData.push_back(particle.pSize);
    }
}

void ParticleSystem::updateInstanceBuffer() {
    instanceData.clear();
    
    for (const auto& particle : particles) {
        if (!particle.isAlive()) continue;
        
        // Position (3 floats)
        instanceData.push_back(particle.pPosition.x);
        instanceData.push_back(particle.pPosition.y);
        instanceData.push_back(particle.pPosition.z);
        
        // Color (3 floats)
        instanceData.push_back(particle.pColor.r);
        instanceData.push_back(particle.pColor.g);
        instanceData.push_back(particle.pColor.b);
        
        // Size (1 float)
        instanceData.push_back(particle.pSize);
        
        // Velocity (3 floats) - used for rotation
        instanceData.push_back(particle.pVelocity.x);
        instanceData.push_back(particle.pVelocity.y);
        instanceData.push_back(particle.pVelocity.z);
    }
}

size_t ParticleSystem::findDeadParticle() {
    // Start from the last known dead particle for efficiency
    for (size_t i = nextDeadParticle; i < particles.size(); ++i) {
        if (!particles[i].isAlive()) {
            nextDeadParticle = i + 1;
            return i;
        }
    }
    
    // Search from the beginning
    for (size_t i = 0; i < nextDeadParticle; ++i) {
        if (!particles[i].isAlive()) {
            nextDeadParticle = i + 1;
            return i;
        }
    }
    
    // No dead particles found
    nextDeadParticle = 0;
    return particles.size();
}

// GroundParticleSystem Implementation
GroundParticleSystem::GroundParticleSystem(size_t maxParticles) {
    particleSystem = std::make_unique<ParticleSystem>(maxParticles);
    particleSystem->setGravity(-0.2f);  // Very light gravity to reach ceiling
    particleSystem->setFadeOut(true);
}

GroundParticleSystem::~GroundParticleSystem() = default;

void GroundParticleSystem::initialize(const Map& map) {
    extractFloorPositions(map);
    std::cout << "Ground particle system initialized with " << floorPositions.size() 
              << " floor spawn points" << std::endl;
}

void GroundParticleSystem::update(float deltaTime) {
    if (!particleSystemEnabled || floorPositions.empty()) {
        particleSystem->update(deltaTime);
        return;
    }
    
    // Update emission timer
    emissionTimer += deltaTime;
    
    // Emit new particles based on emission rate
    float emissionInterval = 1.0f / emissionRate;
    while (emissionTimer >= emissionInterval) {
        emissionTimer -= emissionInterval;
        auto& config = fireConfig;
        switch (currentMode) {
            case GParticleMode::FIRE: {
                // Point to the fire configuration
                config = fireConfig;
                break;
            }
            case GParticleMode::DUST:{
                config = dustConfig;
                break;
            }
        }

        // Spawn single fire particle at random floor position
        glm::vec3 pspawnPos = getRandomFloorPosition();
                
        // Add slight randomness to position 
        pspawnPos.x += randomFloat(-0.5f, 0.5f);
        pspawnPos.z += randomFloat(-0.5f, 0.5f);
        pspawnPos.y += config.particleSpawnHeight;
                
        // Calculate fire velocity (upward with high variation for different heights)
        glm::vec3 pvelocity = glm::vec3(
            randomFloat(-0.5f, 0.5f),  // Horizontal drift
            config.baseVelocity + randomFloat(-1.0 * config.velocityVariation, config.velocityVariation),  // Much more speed variation for height differences
            randomFloat(-0.5f, 0.5f)
        );
                
        // Fixed particle lifetime of 2.5 seconds
        float plife = config.particleLife;
        float psize = config.particleSize + randomFloat(-0.5f, 0.5f);
        float pgravity = config.particleGravity;
        float pfadeRatio = config.particleFadeRatio;
        glm::vec3 pcolor;
        switch (currentMode) {
            case GParticleMode::FIRE: {
                // Point to the fire configuration
                pcolor = calculateFireColor(1.0f);  // Start with full intensity color
                break;
            }
            case GParticleMode::DUST:{
                pcolor = glm::vec3(0.3f, 0.3f, 0.5f);  // Start with full intensity color
                break;
            }
        }
        
                
        particleSystem->emit(pspawnPos, pvelocity, pcolor, plife, psize, pgravity, pfadeRatio);
    }
    
    particleSystem->update(deltaTime);
}

void GroundParticleSystem::render(const glm::mat4& view, const glm::mat4& projection) {
    if (particleSystemEnabled) {
        // Enable additive blending for glowing fire effect
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending
        
        particleSystem->render(view, projection);
        
        // Restore normal blending
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
}

void GroundParticleSystem::extractFloorPositions(const Map& map) {
    floorPositions.clear();
    
    auto floorBrushes = map.getFloorBrushes();
    
    for (const Brush* brush : floorBrushes) {
        // Sample positions across the floor surface
        for (size_t i = 0; i < brush->faces.size(); i += 3) {
            if (i + 2 < brush->faces.size()) {
                uint32_t idx0 = brush->faces[i];
                uint32_t idx1 = brush->faces[i + 1];
                uint32_t idx2 = brush->faces[i + 2];
                
                if (idx0 < brush->vertices.size() && idx1 < brush->vertices.size() && idx2 < brush->vertices.size()) {
                    // Get triangle vertices
                    glm::vec3 v0 = brush->vertices[idx0];
                    glm::vec3 v1 = brush->vertices[idx1];
                    glm::vec3 v2 = brush->vertices[idx2];
                    
                    // Store triangle vertices for real-time random generation
                    floorPositions.push_back(v0);
                    floorPositions.push_back(v1);
                    floorPositions.push_back(v2);
                }
            }
        }
    }
    
    // If no floor positions found, create a default floor area as triangles
    if (floorPositions.empty()) {
        std::cout << "Warning: No floor surfaces found, using default spawn area" << std::endl;
        // Create two triangles forming a default floor (-20 to 20 on x and z axes)
        // Triangle 1
        floorPositions.push_back(glm::vec3(-20.0f, 0.0f, -20.0f));
        floorPositions.push_back(glm::vec3(20.0f, 0.0f, -20.0f));
        floorPositions.push_back(glm::vec3(20.0f, 0.0f, 20.0f));
        // Triangle 2
        floorPositions.push_back(glm::vec3(20.0f, 0.0f, 20.0f));
        floorPositions.push_back(glm::vec3(-20.0f, 0.0f, 20.0f));
        floorPositions.push_back(glm::vec3(-20.0f, 0.0f, -20.0f));
    }
}

glm::vec3 GroundParticleSystem::getRandomFloorPosition() const {
    if (floorPositions.size() < 3) {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }
    
    // Select a random triangle (every 3 vertices form a triangle)
    size_t triangleCount = floorPositions.size() / 3;
    size_t triangleIndex = static_cast<size_t>(randomFloat(0.0f, static_cast<float>(triangleCount - 0.001f)));
    size_t baseIndex = triangleIndex * 3;
    
    // Get triangle vertices
    glm::vec3 v0 = floorPositions[baseIndex];
    glm::vec3 v1 = floorPositions[baseIndex + 1];
    glm::vec3 v2 = floorPositions[baseIndex + 2];
    
    // Generate random point within triangle using barycentric coordinates
    float u = randomFloat(0.0f, 1.0f);
    float v = randomFloat(0.0f, 1.0f);
    
    // Ensure point is within triangle
    if (u + v > 1.0f) {
        u = 1.0f - u;
        v = 1.0f - v;
    }
    
    // Calculate final position
    glm::vec3 point = v0 + u * (v1 - v0) + v * (v2 - v0);
    return point;
}

glm::vec3 GroundParticleSystem::calculateFireColor(float) const {
    // Create vibrant fire colors (white/yellow/orange)
    glm::vec3 color;
    
    // Random selection between white, yellow, and orange
    float colorChoice = randomFloat(0.0f, 3.0f);
    
    if (colorChoice < 1.0f) {
        // White (hot core)
        color = glm::vec3(1.0f, 1.0f, 1.0f);
    } else if (colorChoice < 2.0f) {
        // Yellow
        color = glm::vec3(1.0f, 1.0f, 0.0f);
    } else {
        // Orange
        color = glm::vec3(1.0f, 0.6f, 0.0f);
    }
    
    // Add some color variation
    color.r += randomFloat(-0.1f, 0.1f);
    color.g += randomFloat(-0.1f, 0.1f);
    color.b += randomFloat(-0.05f, 0.1f);
    
    // Add high intensity for strong glowing effect
    float intensity = fireIntensity * (2.0f + randomFloat(-0.3f, 0.3f));
    color *= intensity;
    
    // Ensure minimum brightness
    color = glm::max(color, glm::vec3(0.8f, 0.8f, 0.8f));
    
    return color;  // Don't clamp - allow overbright colors for glow
}

} // namespace silic2