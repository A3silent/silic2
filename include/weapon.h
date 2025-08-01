#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "shader.h"
#include "camera.h"
#include "map.h"

namespace silic2 {

struct Bullet {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 direction;
    float lifetime;
    float maxLifetime;
    float speed;
    glm::vec3 color;
    float intensity;
    float length;  // 子弹长度
    
    Bullet(const glm::vec3& pos, const glm::vec3& dir, float spd = 50.0f) 
        : position(pos), direction(glm::normalize(dir)), speed(spd),
          lifetime(0.0f), maxLifetime(3.0f), 
          color(1.0f, 0.8f, 0.3f), intensity(2.0f), length(0.3f) {
        velocity = direction * speed;
    }
    
    void update(float deltaTime) {
        position += velocity * deltaTime;
        lifetime += deltaTime;
    }
    
    bool isAlive() const {
        return lifetime < maxLifetime;
    }
    
    // 获取子弹作为光源的数据
    glm::vec3 getLightPosition() const {
        return position;
    }
    
    glm::vec3 getLightColor() const {
        return color * intensity;
    }
    
    float getLightRange() const {
        return 3.0f; // 光源范围
    }
};

// 撞击光效结构体
struct ImpactLight {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    float lifetime;     // 剩余生命时间
    float maxLifetime;  // 最大生命时间（用于渐隐）
    
    ImpactLight(const glm::vec3& pos, const glm::vec3& col, float intens) 
        : position(pos), color(col), intensity(intens),
          lifetime(0.2f), maxLifetime(0.2f) {}
    
    void update(float deltaTime) {
        lifetime -= deltaTime;
    }
    
    bool isAlive() const {
        return lifetime > 0.0f;
    }
    
    // 获取渐隐后的强度
    float getFadedIntensity() const {
        return intensity * (lifetime / maxLifetime);
    }
};

class Weapon {
public:
    Weapon();
    ~Weapon();
    
    void init();
    void update(float deltaTime, const Map* map);
    void render(const glm::mat4& view, const glm::mat4& projection);
    
    // 发射子弹，从屏幕右下角向中心射击
    void fire(const Camera& camera);
    
    // 获取活跃子弹的光源数据
    std::vector<std::pair<glm::vec3, glm::vec3>> getActiveLights() const;
    
    // 获取活跃子弹数量
    size_t getActiveBulletCount() const { return bullets.size(); }
    
    // 控制子弹光照效果的开关
    void setBulletLightingEnabled(bool enabled) { bulletLightingEnabled = enabled; }
    bool isBulletLightingEnabled() const { return bulletLightingEnabled; }

private:
    std::vector<Bullet> bullets;
    std::vector<ImpactLight> impactLights;
    std::unique_ptr<Shader> bulletShader;
    std::unique_ptr<Shader> glowShader;
    
    // 子弹网格数据
    unsigned int bulletVAO, bulletVBO;
    // 光晕网格数据
    unsigned int glowVAO, glowVBO;
    
    void setupBulletMesh();
    void setupGlowMesh();
    void cleanupDeadBullets();
    void cleanupDeadLights();
    bool checkBulletCollision(const Bullet& bullet, const Map* map);
    void createImpactLight(const glm::vec3& position, const glm::vec3& color, float intensity);
    
    // 射击冷却
    float fireCooldown;
    float fireRate;
    
    // 子弹光照效果开关
    bool bulletLightingEnabled;
};

} // namespace silic2