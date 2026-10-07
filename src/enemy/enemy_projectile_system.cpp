#include "enemy/enemy_projectile_system.h"
#include "enemy/enemy_geometry.h"
#include <algorithm>

namespace silic2 {
bool EnemyProjectileSystem::spawn(glm::vec3 position,glm::vec3 direction,uint32_t sourceId) {
    if(active.size()>=CAP || glm::length(direction)<1e-6f) return false;
    active.push_back({position,position,glm::normalize(direction),position,sourceId,LIFETIME});
    return true;
}
std::vector<EnemyProjectileHit> EnemyProjectileSystem::update(float dt,const Map* map,const AABB& player) {
    std::vector<EnemyProjectileHit> hits;
    if(dt<=0) return hits;
    const AABB expandedPlayer(player.min-glm::vec3(RADIUS),player.max+glm::vec3(RADIUS));
    for(auto& shot:active) {
        shot.previousPosition=shot.position;
        const auto end=shot.position+shot.direction*SPEED*std::min(dt,shot.remainingLife);
        float nearest=2;
        bool hitPlayer=false;
        if(map) for(const auto& brush:map->getBrushes()) {
            if(brush.vertices.empty()) continue;
            auto box=enemyGeometry::bounds(brush);
            box.min-=glm::vec3(RADIUS); box.max+=glm::vec3(RADIUS);
            float t;
            if(enemyGeometry::segment(shot.position,end,box,t) && t<nearest) nearest=t;
        }
        float t;
        // Wall wins equal-distance ties, preventing hits through wall surfaces.
        if(enemyGeometry::segment(shot.position,end,expandedPlayer,t) && t<nearest) { nearest=t; hitPlayer=true; }
        if(nearest<=1) {
            shot.position=glm::mix(shot.position,end,nearest);
            if(hitPlayer) hits.push_back({shot.position,shot.origin,shot.sourceId,DAMAGE});
            shot.remainingLife=0;
        } else { shot.position=end; shot.remainingLife-=dt; }
    }
    active.erase(std::remove_if(active.begin(),active.end(),[](const auto& p){ return p.remainingLife<=0; }),active.end());
    return hits;
}
}
