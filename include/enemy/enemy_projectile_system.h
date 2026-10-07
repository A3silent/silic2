#pragma once
#include "engine/collision.h"
#include <cstdint>
#include <vector>

namespace silic2 {
class Map;
struct EnemyProjectile {
    glm::vec3 position{}, previousPosition{}, direction{}, origin{};
    uint32_t sourceId=0;
    float remainingLife=4;
};
struct EnemyProjectileHit {
    glm::vec3 position{}, origin{};
    uint32_t sourceId=0;
    float damage=15;
};
class EnemyProjectileSystem {
public:
    static constexpr size_t CAP=32;
    static constexpr float SPEED=10, RADIUS=0.12f, DAMAGE=15, LIFETIME=4;
    bool spawn(glm::vec3 position,glm::vec3 direction,uint32_t sourceId=0);
    std::vector<EnemyProjectileHit> update(float dt,const Map* map,const AABB& player);
    void clear() { active.clear(); }
    size_t size() const { return active.size(); }
    const std::vector<EnemyProjectile>& projectiles() const { return active; }
private:
    std::vector<EnemyProjectile> active;
};
}
