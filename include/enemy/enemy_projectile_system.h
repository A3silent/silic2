#pragma once
#include "engine/collision.h"
#include "game/gameplay_types.h"
#include <cstdint>
#include <vector>

namespace silic2 {
class Map;
struct EnemyProjectile {
    glm::vec3 position{}, previousPosition{}, direction{}, origin{};
    uint32_t sourceId=0;
    float remainingLife=4;
    bool ballistic=false;
    glm::vec3 target{};
    uint32_t volley=0;
};
struct EnemyProjectileHit {
    glm::vec3 position{}, origin{};
    uint32_t sourceId=0;
    float damage=15;
    DamageSource source=DamageSource::EnemyProjectile;
};
class EnemyProjectileSystem {
public:
    static constexpr size_t CAP=32;
    static constexpr float SPEED=10, RADIUS=0.12f, DAMAGE=15, LIFETIME=4;
    bool spawn(glm::vec3 position,glm::vec3 direction,uint32_t sourceId=0);
    bool spawnVolley(glm::vec3 origin,glm::vec3 center,uint32_t sourceId);
    std::vector<EnemyProjectileHit> update(float dt,const Map* map,const AABB& player);
    void clear() { active.clear();volleyDamage.clear();nextVolley=1; }
    size_t activeVolleyCount() const { return volleyDamage.size(); }
    size_t size() const { return active.size(); }
    const std::vector<EnemyProjectile>& projectiles() const { return active; }
private:
    struct VolleyDamage { uint32_t id; float dealt; };
    std::vector<EnemyProjectile> active;
    std::vector<VolleyDamage> volleyDamage;
    uint32_t nextVolley=1;
};
}
