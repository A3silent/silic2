#include "enemy/enemy_projectile_system.h"
#include "enemy/enemy_geometry.h"
#include "enemy/crystal_combat.h"
#include <algorithm>

namespace silic2 {
namespace {
float worldHit(glm::vec3 a,glm::vec3 b,const Map* map,float radius) {
    float nearest=2;
    if(!map) return nearest;
    if(auto world=map->getSpatialWorld()) {
        auto hit=world->raycast(a,b,radius);
        return hit.hit ? hit.fraction : nearest;
    }
    for(const auto& brush:map->getBrushes()) {
        if(brush.vertices.empty()) continue;
        auto box=enemyGeometry::bounds(brush);
        box.min-=glm::vec3(radius);box.max+=glm::vec3(radius);
        float t;
        if(enemyGeometry::segment(a,b,box,t)) nearest=std::min(t,nearest);
    }
    return nearest;
}
glm::vec3 arc(const EnemyProjectile& p,float t) {
    return glm::mix(p.origin,p.target,t)+glm::vec3(0,16.f*t*(1-t),0);
}
}
bool EnemyProjectileSystem::spawn(glm::vec3 position,glm::vec3 direction,uint32_t sourceId) {
    if(active.size()>=CAP || glm::length(direction)<1e-6f) return false;
    active.push_back({position,position,glm::normalize(direction),position,sourceId,LIFETIME});
    return true;
}
bool EnemyProjectileSystem::spawnVolley(glm::vec3 origin,glm::vec3 center,uint32_t sourceId) {
    if(active.size()+3>CAP) return false;
    const uint32_t volley=nextVolley++;
    for(const auto& target:crystalCombat::volleyTargets(center)) {
        EnemyProjectile p;
        p.origin=p.previousPosition=p.position=origin;
        p.target=target;p.sourceId=sourceId;p.volley=volley;p.ballistic=true;
        p.remainingLife=crystalCombat::VolleyTime;
        active.push_back(p);
    }
    volleyDamage.push_back({volley,0});
    return true;
}
std::vector<EnemyProjectileHit> EnemyProjectileSystem::update(float dt,const Map* map,const AABB& player) {
    std::vector<EnemyProjectileHit> hits;
    if(dt<=0) return hits;
    const AABB expandedPlayer(player.min-glm::vec3(RADIUS),player.max+glm::vec3(RADIUS));
    for(auto& shot:active) {
        shot.previousPosition=shot.position;
        if(shot.ballistic) {
            for(float left=dt;left>0 && shot.remainingLife>0;) {
                const float step=std::min({left,shot.remainingLife,1.f/60});
                shot.remainingLife-=step;left-=step;
                const auto end=arc(shot,std::clamp(1-shot.remainingLife/crystalCombat::VolleyTime,0.f,1.f));
                float wall=worldHit(shot.position,end,map,RADIUS);
                const auto prev=shot.position;
                shot.position=wall<=1 ? glm::mix(prev,end,wall) : end;
                if(wall>1 && shot.remainingLife>.000001f) continue;
                shot.remainingLife=0;
                const auto closest=glm::clamp(shot.position,player.min,player.max);
                // A swept sphere stops on the incoming side of an occluder;
                // its blast can't reach through the crystal into the other side.
                if(glm::length(closest-shot.position)>crystalCombat::BurstRadius ||
                   !enemyGeometry::clear(shot.position,player.getCenter(),map)) break;
                auto group=std::find_if(volleyDamage.begin(),volleyDamage.end(),[&](auto& g){return g.id==shot.volley;});
                if(group!=volleyDamage.end()) {
                    const float damage=std::min(crystalCombat::OrbDamage,crystalCombat::VolleyDamageLimit-group->dealt);
                    if(damage>0) {
                        hits.push_back({shot.position,shot.origin,shot.sourceId,damage,DamageSource::CenserBurst});
                        group->dealt+=damage;
                    }
                }
            }
            continue;
        }
        const auto end=shot.position+shot.direction*SPEED*std::min(dt,shot.remainingLife);
        float nearest=worldHit(shot.position,end,map,RADIUS);
        bool hitPlayer=false;
        float t;
        if(enemyGeometry::segment(shot.position,end,expandedPlayer,t) && t<nearest) { nearest=t; hitPlayer=true; }
        if(nearest<=1) {
            shot.position=glm::mix(shot.position,end,nearest);
            if(hitPlayer) hits.push_back({shot.position,shot.origin,shot.sourceId,DAMAGE});
            shot.remainingLife=0;
        } else { shot.position=end; shot.remainingLife-=dt; }
    }
    active.erase(std::remove_if(active.begin(),active.end(),[](const auto& p){ return p.remainingLife<=0; }),active.end());
    volleyDamage.erase(std::remove_if(volleyDamage.begin(),volleyDamage.end(),[&](const auto& group){
        return std::none_of(active.begin(),active.end(),[&](const auto& p){return p.ballistic && p.volley==group.id;});
    }),volleyDamage.end());
    return hits;
}
}
