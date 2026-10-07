#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "engine/shader.h"
#include "engine/camera.h"
#include "engine/map.h"
#include "game/gameplay_types.h"

namespace silic2 {

class EnemyManager; // forward declaration

struct WeaponUpdateResult {
    int hits = 0;
    int kills = 0;
    std::vector<EnemyType> killedEnemyTypes;
};

struct Bullet {
    glm::vec3 position;
    glm::vec3 prevPosition;  // Position last frame — used for swept collision
    glm::vec3 velocity;
    glm::vec3 direction;
    float lifetime;
    float maxLifetime;
    float speed;
    glm::vec3 color;
    float intensity;
    float length;  // Bullet length

    Bullet(const glm::vec3& pos, const glm::vec3& dir, float spd = 50.0f)
        : position(pos), prevPosition(pos), direction(glm::normalize(dir)),
          lifetime(0.0f), maxLifetime(3.0f), speed(spd),
          color(0.5f, 0.8f, 1.0f), intensity(2.0f), length(0.3f) {  // Sky blue color
        velocity = direction * speed;
    }

    void update(float deltaTime) {
        prevPosition = position;
        position += velocity * deltaTime;
        lifetime += deltaTime;
    }
    
    bool isAlive() const {
        return lifetime < maxLifetime;
    }
    
    // Get bullet data as light source
    glm::vec3 getLightPosition() const {
        return position;
    }
    
    glm::vec3 getLightColor() const {
        return color * intensity;
    }
    
    float getLightRange() const {
        return 3.0f; // Light range
    }
};

// Impact light effect structure
struct ImpactLight {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    float lifetime;     // Remaining lifetime
    float maxLifetime;  // Maximum lifetime (for fading)
    
    ImpactLight(const glm::vec3& pos, const glm::vec3& col, float intens) 
        : position(pos), color(col), intensity(intens),
          lifetime(0.2f), maxLifetime(0.2f) {}
    
    void update(float deltaTime) {
        lifetime -= deltaTime;
    }
    
    bool isAlive() const {
        return lifetime > 0.0f;
    }
    
    // Get faded intensity
    float getFadedIntensity() const {
        return intensity * (lifetime / maxLifetime);
    }
};

class Weapon {
public:
    Weapon();
    ~Weapon();
    
    void init();
    WeaponUpdateResult update(float deltaTime, const Map* map,
                              EnemyManager* enemies = nullptr);
    void render(const glm::mat4& view, const glm::mat4& projection);
    
    // Fire bullet from screen bottom-right towards center
    bool fire(const Camera& camera);
    void reset();
    
    // Get active bullets' light source data
    std::vector<std::pair<glm::vec3, glm::vec3>> getActiveLights() const;
    
    // Get active bullet count
    size_t getActiveBulletCount() const { return bullets.size(); }
    size_t getActiveImpactLightCount() const { return impactLights.size(); }
    
    // Control bullet lighting effects toggle
    void setBulletLightingEnabled(bool enabled) { bulletLightingEnabled = enabled; }
    bool isBulletLightingEnabled() const { return bulletLightingEnabled; }

private:
    std::vector<Bullet> bullets;
    std::vector<ImpactLight> impactLights;
    std::unique_ptr<Shader> bulletShader;
    std::unique_ptr<Shader> glowShader;
    
    // Bullet mesh data
    unsigned int bulletVAO, bulletVBO;
    // Glow mesh data
    unsigned int glowVAO, glowVBO;
    
    void setupBulletMesh();
    void setupGlowMesh();
    void cleanupDeadBullets();
    void cleanupDeadLights();
    bool checkBulletCollision(const Bullet& bullet, const Map* map);
    void createImpactLight(const glm::vec3& position, const glm::vec3& color, float intensity);
    
    // Fire cooldown
    float fireCooldown;
    float fireRate;
    
    // Bullet lighting effects toggle
    bool bulletLightingEnabled;
};

} // namespace silic2
