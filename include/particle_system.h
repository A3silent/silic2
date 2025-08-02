#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <glad/glad.h>
#include "map.h"

namespace silic2 {

class Shader;

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
    float life;         // Current life (0.0 = dead, 1.0 = full life)
    float maxLife;      // Maximum life duration
    float size;         // Particle size
    float gravity;      // Gravity effect multiplier
    
    Particle() : position(0.0f), velocity(0.0f), color(1.0f), 
                 life(0.0f), maxLife(1.0f), size(1.0f), gravity(1.0f) {}
    
    bool isAlive() const { return life > 0.0f; }
};

class ParticleSystem {
public:
    ParticleSystem(size_t maxParticles = 1000);
    ~ParticleSystem();

    // Update particles
    void update(float deltaTime);
    
    // Render particles
    void render(const glm::mat4& view, const glm::mat4& projection);
    
    // Emit particles
    void emit(const glm::vec3& position, const glm::vec3& velocity, 
              const glm::vec3& color, float life, float size = 1.0f);
    
    // Batch emit particles (for efficiency)
    void emitBurst(const glm::vec3& position, int count, 
                   const glm::vec3& baseVelocity, const glm::vec3& velocityVariation,
                   const glm::vec3& color, float life, float size = 1.0f);
    
    // Settings
    void setGravity(float gravity) { defaultGravity = gravity; }
    void setWindForce(const glm::vec3& wind) { windForce = wind; }
    void setFadeOut(bool enabled) { fadeOutEnabled = enabled; }
    
    // Statistics
    size_t getActiveParticles() const;
    size_t getMaxParticles() const { return maxParticleCount; }

private:
    std::vector<Particle> particles;
    size_t maxParticleCount;
    size_t nextDeadParticle;
    
    // Rendering resources
    GLuint VAO, VBO;
    GLuint boxVAO, boxVBO;        // Box mesh for 3D particles
    GLuint instanceVBO;           // Instance data buffer
    std::unique_ptr<Shader> particleShader;
    std::unique_ptr<Shader> boxShader;      // Shader for 3D box particles
    bool use3DBoxes = true;       // Toggle between point sprites and 3D boxes
    
    // Physics settings
    float defaultGravity = -9.8f;
    glm::vec3 windForce = glm::vec3(0.0f);
    bool fadeOutEnabled = true;
    
    // Rendering buffer
    std::vector<float> vertexData;
    std::vector<float> instanceData;  // For instanced rendering
    
    void initRenderingResources();
    void setupBoxMesh();
    void updateVertexBuffer();
    void updateInstanceBuffer();
    size_t findDeadParticle();
};

class GroundParticleSystem {
public:
    GroundParticleSystem(size_t maxParticles = 500);
    ~GroundParticleSystem();
    
    // Initialize with map floor surfaces
    void initialize(const Map& map);
    
    // Update and render
    void update(float deltaTime);
    void render(const glm::mat4& view, const glm::mat4& projection);
    
    // Configuration
    void setEmissionRate(float particlesPerSecond) { emissionRate = particlesPerSecond; }
    void setFireIntensity(float intensity) { fireIntensity = intensity; }
    void setEnabled(bool enabled) { particleSystemEnabled = enabled; }
    
    bool isEnabled() const { return particleSystemEnabled; }

private:
    std::unique_ptr<ParticleSystem> particleSystem;
    std::vector<glm::vec3> floorPositions;  // Potential spawn points on floor surfaces
    
    // Emission settings
    float emissionRate = 50.0f;  // particles per second
    float emissionTimer = 0.0f;
    float fireIntensity = 1.0f;
    bool particleSystemEnabled = true;
    
    // Fire effect parameters
    struct FireConfig {
        glm::vec3 baseColor = glm::vec3(1.0f, 0.2f, 0.0f);      // Deep orange-red
        glm::vec3 tipColor = glm::vec3(1.0f, 1.0f, 0.0f);       // Bright yellow
        float baseVelocity = 7.0f;         // Much higher upward velocity to reach ceiling
        float velocityVariation = 2.0f;    // More variation for different end heights
        float particleLife = 2.5f;         // 2.5 seconds lifetime
        float particleSize = 4.0f;         // Slightly larger for better visibility
        float spawnHeight = 0.01f;         // Very close to ground
    } fireConfig;
    
    void extractFloorPositions(const Map& map);
    glm::vec3 getRandomFloorPosition() const;
    glm::vec3 calculateFireColor(float lifeRatio) const;
};

} // namespace silic2