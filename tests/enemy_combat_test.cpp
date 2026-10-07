#include "enemy/enemy.h"
#include "enemy/enemy_projectile_system.h"
#include "engine/map.h"
#include "game/arena_definition.h"
#include "test_support.h"
#include <cmath>

using namespace silic2;
using silic2::test::require;
using silic2::test::requireNear;
namespace {
Brush box(glm::vec3 low,glm::vec3 high) {
    static uint32_t nextId=1;
    Brush result; result.id=nextId++;
    result.vertices={low,{high.x,low.y,low.z},{high.x,high.y,low.z},{low.x,high.y,low.z},
                     {low.x,low.y,high.z},{high.x,low.y,high.z},high,{low.x,high.y,high.z}};
    result.faces={0,2,1,0,3,2,4,5,6,4,6,7,0,4,7,0,7,3,1,2,6,1,6,5,3,7,6,3,6,2,0,1,5,0,5,4};
    return result;
}
Map floorMap() {
    Map map; require(map.addBrush(box({-30,-1,-30},{30,0,30})), "valid collision fixture: "+map.getLastError()); return map;
}
int countActions(const Enemy& enemy,EnemyAction::Kind kind) {
    int result=0; for(const auto& action:enemy.actions()) if(action.kind==kind) ++result;
    return result;
}
void sentryTiming() {
    auto map=floorMap();
    Enemy sentry({0,0.05f,0},12,EnemyType::Sentry); sentry.setId(7);
    sentry.update(.3f,{0,0,10},&map);
    require(sentry.isCharging() && countActions(sentry,EnemyAction::Kind::Charge)==1,"visible sentry begins charging once");
    sentry.update(.3f,{5,0,10},&map);
    require(sentry.getHeading().x>0.4f,"sentry tracks during first .65 seconds");
    sentry.update(.06f,{5,0,10},&map);
    require(sentry.isAimLocked(),"sentry locks at .65 seconds");
    const auto locked=sentry.getHeading();
    sentry.update(.2f,{-5,0,10},&map);
    require(glm::length(sentry.getHeading()-locked)<1e-5f,"locked aim does not follow player");
    sentry.update(.15f,{-5,0,10},&map);
    require(countActions(sentry,EnemyAction::Kind::Fire)==1,"one projectile emitted at charge completion");
    for(const auto& event:sentry.actions()) if(event.kind==EnemyAction::Kind::Fire) {
        require(event.sourceId==7 && glm::length(event.direction-locked)<1e-5f,"fire action retains source ID and locked aim");
    }
    require(sentry.getState()==EnemyState::RECOVERING,"sentry recovers after fire");
    sentry.update(2.18f,{0,0,10},&map);
    require(!sentry.isCharging(),"recovery lasts 2.2 seconds after actual fire time");
    sentry.update(.03f,{0,0,10},&map);
    require(sentry.isCharging(),"sentry recharges after recovery");
    const float progress=sentry.getChargeProgress();
    sentry.update(0,{0,0,10},&map);
    requireNear(sentry.getChargeProgress(),progress,1e-6f,"zero delta freezes charging");
    sentry.takeDamage(1);
    require(sentry.getHitFlash()==1 && sentry.getSecondsSinceDamage()==0,"damage exposes distinct hit cue");
    sentry.takeDamage(100);
    sentry.update(3,{0,0,10},&map);
    require(sentry.actions().empty(),"dead sentry never emits attacks");
}
void sentryVisibility() {
    auto map=floorMap(); require(map.addBrush(box({-2,0,4},{2,3,5})), "valid collision fixture: "+map.getLastError());
    Enemy sentry({0,.05f,0},12,EnemyType::Sentry);
    sentry.update(.8f,{0,0,10},&map);
    require(!sentry.isCharging(),"occlusion prevents charge start");
    auto clear=floorMap();
    sentry.update(.8f,{0,0,10},&clear);
    sentry.update(.21f,{0,0,10},&map);
    require(countActions(sentry,EnemyAction::Kind::Fire)==0 && countActions(sentry,EnemyAction::Kind::CancelCharge)==1,"firing path rechecked after cover intervenes");
    require(!sentry.isTouchingPlayer({0,0,0}),"sentry has no chaser contact damage");
    requireNear(sentry.getAABB().getSize().y,1,1e-6f,"sentry uses shared type dimensions");
}
void sentryFrameRates() {
    for(float step:{1.0f/30,1.0f/60,1.0f/144,0.0f}) {
        auto map=floorMap(); Enemy sentry({0,.05f,0},12,EnemyType::Sentry);
        const float irregular[]={.017f,.093f,.003f,.041f};
        float time=0; int frame=0,shots=0;
        while(time<10) {
            const float dt=std::min(10-time,step>0 ? step : irregular[frame++%4]);
            sentry.update(dt,{0,0,10},&map);
            shots+=countActions(sentry,EnemyAction::Kind::Fire); time+=dt;
        }
        require(shots==3,"sentry cadence survives 30/60/144 FPS and irregular steps");
    }
}
void projectileSweeps() {
    EnemyProjectileSystem system;
    const AABB player({8,-1,-1},{9,1,1});
    Map map; require(map.addBrush(box({4,-2,-2},{4.1f,2,2})), "valid collision fixture: "+map.getLastError());
    require(system.spawn({0,0,0},{1,0,0},11),"projectile spawn");
    require(system.update(1,&map,player).empty() && system.size()==0,"nearest wall blocks swept player hit");
    map=Map(); require(map.addBrush(box({12,-2,-2},{13,2,2})), "valid collision fixture: "+map.getLastError());
    system.spawn({0,0,0},{1,0,0},12);
    auto hits=system.update(2,&map,player);
    require(hits.size()==1 && hits[0].sourceId==12 && hits[0].damage==15,"near player beats farther wall");
    requireNear(hits[0].position.x,7.88f,.001f,"projectile radius expands target");
    require(glm::length(hits[0].origin)<1e-6f,"hit reports source origin for damage direction");
    // Radius catches a grazing collision even though the center line misses.
    system.spawn({0,1.1f,0},{1,0,0});
    require(system.update(1,nullptr,player).size()==1,"radius catches player graze");
    for(int i=0;i<32;++i) require(system.spawn({0,5,0},{1,0,0}),"projectile cap allows 32");
    require(!system.spawn({0,5,0},{1,0,0}),"projectile cap rejects 33rd");
    const AABB distant({100,100,100},{101,101,101});
    system.update(0,nullptr,distant); require(system.size()==32,"pause preserves projectiles");
    system.update(4.1f,nullptr,distant); require(system.size()==0,"lifetime removes projectiles");
    // No collision may occur beyond a projectile's four-second range.
    system.spawn({0,0,0},{1,0,0});
    require(system.update(6,nullptr,AABB({45,-1,-1},{46,1,1})).empty(),"long delta cannot exceed lifetime travel");
    system.spawn({0,0,0},{1,0,0}); system.clear(); require(system.size()==0,"restart clears projectiles");
}
void chaserNavigation() {
    auto map=floorMap(); const auto arena=ArenaDefinition::starryCrystal();
    for(const auto& obstacle:arena.obstacles) require(map.addBrush(box(obstacle.minimum,obstacle.maximum)), "valid collision fixture: "+map.getLastError());
    for(float dt:{1.0f/30,1.0f/60,1.0f/144}) {
        Enemy chaser({0,.05f,-15},6); chaser.setArenaDefinition(std::make_shared<ArenaDefinition>(arena));
        Enemy replay=chaser;
        for(int i=0;i<static_cast<int>(20/dt);++i) {
            chaser.update(dt,{0,0,13},&map); replay.update(dt,{0,0,13},&map);
            require(arena.isWalkable(chaser.getPosition(),.3f),"chaser stays clear of arena blockers");
        }
        require(glm::length(glm::vec2(chaser.getPosition().x,chaser.getPosition().z-13))<.5f,"chaser routes around center from arena-wide range");
        require(glm::length(chaser.getPosition()-replay.getPosition())<1e-6f,"navigation is deterministic for identical input");
    }
}
void chaserEdgeNavigation() {
    auto map=floorMap(); const auto arena=ArenaDefinition::starryCrystal();
    for(const auto& obstacle:arena.obstacles) require(map.addBrush(box(obstacle.minimum,obstacle.maximum)), "valid collision fixture: "+map.getLastError());
    // Include physical arena walls: a route must recover without escaping the map.
    require(map.addBrush(box({18,0,-19},{19,6,19})), "valid collision fixture: "+map.getLastError());
    require(map.addBrush(box({-19,0,-19},{-18,6,19})), "valid collision fixture: "+map.getLastError());
    require(map.addBrush(box({-18,0,18},{18,6,19})), "valid collision fixture: "+map.getLastError());
    require(map.addBrush(box({-18,0,-19},{18,6,-18})), "valid collision fixture: "+map.getLastError());
    const float irregular[]={.017f,.043f,.009f,.031f};
    for(float step:{1.0f/30,1.0f/60,1.0f/144,0.0f}) {
        auto pursue=[&](Enemy& enemy,const glm::vec3& target,const char* failure) {
            float elapsed=0; int frame=0;
            while(elapsed<20) {
                const float dt=std::min(20-elapsed,step>0 ? step : irregular[frame++%4]);
                enemy.update(dt,target,&map); elapsed+=dt;
                require(arena.isWalkable(enemy.getPosition(),.3f),"edge recovery keeps physical body outside blockers and walls");
            }
            const auto position=enemy.getPosition();
            require(glm::length(glm::vec2(position.x-target.x,position.z-target.z))<.5f,failure);
        };
        // Both points are legal for the .30 physical radius but outside .34 nav clearance.
        for(const glm::vec3 target:{glm::vec3(2.32f,0,0),glm::vec3(17.68f,0,0)}) {
            require(arena.isWalkable(target,.3f) && !arena.isWalkable(target,.34f),"edge target exercises physical/nav clearance mismatch");
            Enemy chaser({0,.05f,-15},6); chaser.setArenaDefinition(std::make_shared<ArenaDefinition>(arena));
            pursue(chaser,target,"chaser approaches player hugging pillar or outer wall");
        }
        // Reproduce separation entering the narrow .30-.34 strip during real movement.
        for(bool wall:{false,true}) {
            Enemy chaser({wall ? 17.65f : 2.35f,.05f,0},6); chaser.setArenaDefinition(std::make_shared<ArenaDefinition>(arena));
            chaser.setSeparation({wall ? 1.2f : -1.2f,0,0});
            chaser.update(.025f,{wall ? 17.65f : 2.35f,0,10},&map);
            require(arena.isWalkable(chaser.getPosition(),.3f) && !arena.isWalkable(chaser.getPosition(),.34f),"separation places chaser in physical-only clearance strip");
            chaser.setSeparation({0,0,0});
            pursue(chaser,{0,0,13},"chaser recovers from separation-induced clearance mismatch");
        }
        // Projection must also beat the actor's .01 movement deadzone.
        for(float edgeX:{2.335f,17.665f}) {
            Enemy chaser({edgeX,.05f,0},6); chaser.setArenaDefinition(std::make_shared<ArenaDefinition>(arena));
            pursue(chaser,{0,0,13},"near-clearance chaser does not stall inside movement deadzone");
        }
    }
}

void chaserCoverTopNavigation() {
    auto map=floorMap(); const auto arena=ArenaDefinition::starryCrystal();
    for(const auto& obstacle:arena.obstacles) require(map.addBrush(box(obstacle.minimum,obstacle.maximum)), "valid collision fixture: "+map.getLastError());
    require(map.addBrush(box({18,0,-19},{19,6,19})), "valid collision fixture: "+map.getLastError());
    require(map.addBrush(box({-19,0,-19},{-18,6,19})), "valid collision fixture: "+map.getLastError());
    require(map.addBrush(box({-18,0,18},{18,6,19})), "valid collision fixture: "+map.getLastError());
    require(map.addBrush(box({-18,0,-19},{18,6,-18})), "valid collision fixture: "+map.getLastError());
    const float irregular[]={.017f,.043f,.009f,.031f};
    for(float step:{1.0f/30,1.0f/60,1.0f/144,0.0f}) {
        for(float coverX:{-9.0f,9.0f}) for(float topY:{1.4f,1.5f}) {
            const glm::vec3 topTarget(coverX,topY,0);
            for(const auto& rift:arena.rifts) {
                Enemy chaser(rift+glm::vec3(0,.05f,0),6); chaser.setArenaDefinition(std::make_shared<ArenaDefinition>(arena));
                auto advance=[&](const glm::vec3& target) {
                    float elapsed=0; int frame=0;
                    while(elapsed<20) {
                        const float dt=std::min(20-elapsed,step>0 ? step : irregular[frame++%4]);
                        chaser.update(dt,target,&map); elapsed+=dt;
                        require(arena.isWalkable(chaser.getPosition(),.3f),"cover-top pursuit never crosses cover or arena walls");
                        requireNear(chaser.getPosition().y,.05f,.001f,"ground chaser cannot climb through cover while pursuing elevated player");
                    }
                };
                advance(topTarget);
                const auto position=chaser.getPosition();
                const glm::vec2 offset(position.x-coverX,position.z);
                const glm::vec2 outside=glm::max(glm::abs(offset)-glm::vec2(1,2),glm::vec2(0));
                require(glm::length(outside)<.5f && glm::length(offset)<2.6f,
                        "chaser from every rift reaches a cover neighbor while player stands on top");
                const glm::vec3 groundedTarget(0,0,13);
                advance(groundedTarget);
                const auto returned=chaser.getPosition();
                require(glm::length(glm::vec2(returned.x,returned.z-13))<.5f,
                        "chaser resumes direct pursuit after player leaves cover top for ground");
            }
        }
    }
}

}
void runEnemyCombatTests() {
    sentryTiming(); sentryVisibility(); sentryFrameRates(); projectileSweeps(); chaserNavigation(); chaserEdgeNavigation(); chaserCoverTopNavigation();
}
