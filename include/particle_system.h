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
    glm::vec3 baseColor;
    float life;         // Current life (0.0 = dead, 1.0 = full life)
    float maxLife;      // Maximum life duration
    float size;         // Particle size
    float gravity;      // Gravity effect multiplier

    float fadeRatio;
    float brightnessRatio;
    
    Particle() : position(0.0f), velocity(0.0f), color(1.0f), baseColor(1.0f),
                 life(0.0f), maxLife(1.0f), size(1.0f), gravity(1.0f), fadeRatio(1.0f){}
    
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
              const glm::vec3& color, float life, float size , float gravity, float fadeRatio = 1.0f);
    
    // Batch emit particles (for efficiency)
    void emitBurst(const glm::vec3& position, int count, 
                   const glm::vec3& baseVelocity, const glm::vec3& velocityVariation,
                   const glm::vec3& color, float life, float size , float gravity, float fadeRatio = 1.0f);

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

    //Fade Settings
    //A lookup Hash Table that stores precalculated values for lifeRatio^fadeRatio
    //This way we prevent calculating std::pow on every particle
    std::unordered_map<float, std::vector<float>> fadeLUTCache;
    float lastFadeRatio = -1.0f;
    int fadeOutSmoothness = 32; //Higher val = smoother fade transition
    
    // Rendering buffer
    std::vector<float> vertexData;
    std::vector<float> instanceData;  // For instanced rendering
    
    void initRenderingResources();
    void setupBoxMesh();
    void updateVertexBuffer();
    void updateInstanceBuffer();
    size_t findDeadParticle();
    const std::vector<float>& getFadeLUT(float fadeRatio);
};

class GroundParticleSystem {
public:
    enum class particleMode{
        FIRE,
        DUST
    };

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
    void setParticleMode(particleMode mode){ currentMode = mode; }
    particleMode getParticleMode() const { return currentMode; }
    
    bool isEnabled() const { return particleSystemEnabled; }

private:
    std::unique_ptr<ParticleSystem> particleSystem;
    std::vector<glm::vec3> floorPositions;  // Potential spawn points on floor surfaces
    
    // Emission settings
    float emissionRate = 50.0f;  // particles per second
    float emissionTimer = 0.0f;
    float fireIntensity = 1.0f;
    bool particleSystemEnabled = true;
    particleMode currentMode; //Current Particle Mode
    
    struct ParticleConfig {
        glm::vec3 baseColor;
        glm::vec3 tipColor;  // for dust you can ignore or set default
        float baseVelocity;
        float velocityVariation;
        float particleLife;
        float particleSize;
        float spawnHeight;
        float particleGravity;
        float particleFadeRatio;
    };

    ParticleConfig fireConfig = {
        glm::vec3(1.0f, 0.2f, 0.0f),
        glm::vec3(1.0f, 1.0f, 0.0f),
        7.0f,
        2.0f,
        2.5f,
        4.0f,
        0.01f,
        1.0f,
        40.0f
    };

    ParticleConfig dustConfig = {
        glm::vec3(0.8f, 0.7f, 0.6f), // baseColor for dust
        glm::vec3(0.8f, 0.7f, 0.6f), // tipColor (same as base)
        1.0f,
        0.2f,
        5.0f,
        4.0f,
        0.01f,
        0.3f,
        3.0f
    };

    void extractFloorPositions(const Map& map);
    glm::vec3 getRandomFloorPosition() const;
    glm::vec3 calculateFireColor(float lifeRatio) const;
};

}