#include "enemy/enemy.h"
#include "enemy/enemy_manager.h"
#include "enemy/crystal_combat.h"
#include "enemy/enemy_projectile_system.h"
#include "engine/map.h"
#include "game/arena_definition.h"
#include "game/encounter_director.h"
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

void crystalAttacks() {
    auto floor=floorMap();
    for(float dt:{1.f/30,1.f/60,1.f/144,.041f}) {
        Enemy hound({0,.05f,9},18,EnemyType::ForkHound);
        int warnings=0, strikes=0;
        for(float t=0;t<1.9f;t+=dt) {
            hound.update(dt,{0,0,0},&floor);
            warnings+=countActions(hound,EnemyAction::Kind::Windup);
            for(auto& a:hound.actions()) if(a.kind==EnemyAction::Kind::Strike) {
                ++strikes;requireNear(a.damage,18,.01f,"dash damage");
            }
            require(!hound.isTouchingPlayer(hound.getPosition()),"hound has no passive contact damage");
        }
        require(warnings==1 && strikes==1,"one warned dash causes one swept hit at every frame rate");
        Enemy ray({0,.04f,-18},14,EnemyType::VeilRay);
        int sweeps=0;bool warned=false;bool avoidedReturn=false;
        glm::vec3 stripStart{}, stripEnd{}, target{};
        for(float t=0;t<4;t+=dt) {
            ray.update(dt,target,&floor);
            if(countActions(ray,EnemyAction::Kind::Windup)) {
                warned=true;stripStart=ray.getSweepStart();stripEnd=ray.getSweepEnd();
                requireNear(glm::length(stripEnd-stripStart),14,.001f,"ray warning is exactly attack length");
                target=stripStart+glm::vec3(4,0,0);
            }
            if(ray.getAttackPhase()==AttackPhase::Attack && ray.getPhaseProgress()>.4f && !avoidedReturn) {
                target=glm::mix(stripStart,stripEnd,.05f);
                avoidedReturn=true;
            }
            sweeps+=countActions(ray,EnemyAction::Kind::Strike);
        }
        require(warned && avoidedReturn && sweeps==0,"already swept first section is safe on reentry");
        Enemy liveRay({0,.04f,-18},14,EnemyType::VeilRay);
        for(float t=0;t<4;t+=dt) {
            liveRay.update(dt,{0,0,0},&floor);
            sweeps+=countActions(liveRay,EnemyAction::Kind::Strike);
        }
        require(sweeps==1,"stationary target inside strip is hit once");
    }
}

void scaledFlyingBodies() {
    Enemy ray({0,5,0},14,EnemyType::VeilRay);
    requireNear(ray.getDimensions().x,8.75f,.001f,"ray has an 8.75 metre wingspan");
    Enemy hound({0,0,0},18,EnemyType::ForkHound);
    requireNear(hound.getDimensions().z,2.8f,.001f,"hound length is unchanged");
    // With default -Z heading, positive model Z points toward world -Z.
    float t=0;bool gland=false;
    require(ray.intersectShot({3.88f,5.48f,-3},{3.88f,5.48f,3},t,gland) && !gland,
            "the enlarged outboard wing is hittable well beyond the previous silhouette");
    require(!ray.intersectShot({4.6f,5.48f,-3},{4.6f,5.48f,3},t,gland),
            "a shot outside the enlarged tip still misses");
}
void lockedHoundAndBlockedLane() {
    const auto floor=floorMap();
    auto blocked=floorMap();
    require(blocked.addBrush(box({-3,0,2},{3,4,2.5f})),"add dash obstacle");
    for(float dt:{1.f/30,1.f/144,.047f}) {
        Enemy dog({0,.05f,11},18,EnemyType::ForkHound);
        for(int i=0;i<400 && dog.getAttackPhase()!=AttackPhase::Windup;++i) dog.update(dt,{0,0,0},&floor);
        require(dog.getAttackPhase()==AttackPhase::Windup,"hound finds clear dash lane");
        while(dog.getPhaseProgress()<.55f) dog.update(dt,{0,0,0},&floor);
        const glm::vec3 locked=dog.getHeading();
        auto againstWall=dog;
        int hits=0, wallHits=0;
        for(float t=0;t<1.2f;t+=dt) {
            dog.update(dt,{5,0,0},&floor);
            againstWall.update(dt,{0,0,0},&blocked);
            require(glm::length(dog.getHeading()-locked)<.001f,"locked hound cannot steer into a dodge");
            hits+=countActions(dog,EnemyAction::Kind::Strike);
            wallHits+=countActions(againstWall,EnemyAction::Kind::Strike);
        }
        require(!hits && !wallHits && againstWall.getPosition().z>2.f,"a sidestep or solid landmark prevents dash damage");
    }
    auto obstruction=floorMap();
    require(obstruction.addBrush(box({-2,0,-1},{2,12,1})),"add tall central obstacle");
    Enemy ray({0,5,-12},14,EnemyType::VeilRay);
    bool acquired=false;
    for(float t=0;t<10;t+=1.f/60) {
        ray.update(1.f/60,{3.8f,0,0},&obstruction);
        if(countActions(ray,EnemyAction::Kind::Windup)) {
            acquired=true;
            require(ray.getSweepStart().x>2 || ray.getSweepEnd().x>2,"ray can stage a lane beside the obstacle");
            break;
        }
    }
    require(acquired,"ray searches alternate low-altitude run beside obstructing crystal");

    Map crystal;
    require(crystal.loadFromFile("res/maps/starry_crystal_arena.json"),"load production Crystal rim");
    auto arena=std::make_shared<ArenaDefinition>(ArenaDefinition::fromMap(crystal));
    for(const auto& rift:arena->rifts) for(auto type:{EnemyType::VeilRay,EnemyType::HangingCenser}) {
        Enemy flyer(rift+glm::vec3(0,.015f,0),24,type);
        flyer.setArenaDefinition(arena);
        const auto player=rift+glm::vec3(20,0,0);
        flyer.update(1.f,player,&crystal);
        const float altitude=type==EnemyType::VeilRay ? crystalCombat::RayAltitude : crystalCombat::CenserAltitude;
        require(flyer.getPosition().y>=arena->floorY+altitude-.25f,
                "enlarged flyer lifts off from every actual inner and outer rift");
    }
    for(float radius:{95.f,98.5f}) {
        Enemy edgeRay({radius-24,.015f,1},14,EnemyType::VeilRay);
        edgeRay.setArenaDefinition(arena);
        edgeRay.setId(1);
        int hits=0;
        for(int frame=0;frame<600;++frame) {
            edgeRay.update(1.f/60,{radius,0,0},&crystal);
            hits+=countActions(edgeRay,EnemyAction::Kind::Strike);
        }
        require(hits>0,"full-width ray can hit a player following the actual low rock rim");
    }
    Enemy orbitRay({40,.05f,0},14,EnemyType::VeilRay);
    orbitRay.setId(7);
    orbitRay.setArenaDefinition(arena);
    int orbitAttacks=0;
    for(int frame=0;frame<420;++frame) {
        constexpr float dt=1.f/30;
        const float angle=frame*dt;
        const glm::vec3 player(10*std::cos(angle),.02f,10*std::sin(angle));
        const glm::vec3 next(10*std::cos(angle+dt),.02f,10*std::sin(angle+dt));
        orbitRay.setAttackContext(true,(next-player)/dt,{player-glm::vec3(.3f,0,.3f),player+glm::vec3(.3f,1.8f,.3f)});
        orbitRay.update(dt,player,&crystal);
        orbitAttacks+=countActions(orbitRay,EnemyAction::Kind::Release);
    }
    require(orbitAttacks>0,"full-width ray finds an attack route around the moving player and central spire");
    Enemy rimHound({71,.015f,1},18,EnemyType::ForkHound);
    rimHound.setId(1);
    rimHound.setArenaDefinition(arena);
    int rimWarnings=0;
    for(int frame=0;frame<1200;++frame) {
        rimHound.update(1.f/60,{95,0,0},&crystal);
        rimWarnings+=countActions(rimHound,EnemyAction::Kind::Windup);
    }
    require(rimWarnings>=3,"hound can turn along the rock rim and repeatedly attack a stationary edge player");
}

void houndFlankFacing() {
    const auto floor=floorMap();
    for(float dt:{1.f/30,1.f/60,1.f/144}) {
        for(float playerSpeed:{0.f,6.f}) {
            Enemy dog({0,.02f,6.05f},9,EnemyType::ForkHound);
            dog.setId(1);
            glm::vec3 player(0), previousHeading=dog.getHeading();
            float distanceTravelled=0;
            bool warned=false;
            for(float t=0;t<4.0f;t+=dt) {
                player.x+=playerSpeed*dt;
                dog.setAttackContext(t>=3.f,{playerSpeed,0,0},
                                     {player-glm::vec3(.3f,0,.3f),player+glm::vec3(.3f,1.8f,.3f)});
                const glm::vec3 before=dog.getPosition();
                dog.update(dt,player,&floor);
                const glm::vec3 heading=dog.getHeading();
                const float turn=std::acos(glm::clamp(glm::dot(heading,previousHeading),-1.f,1.f));
                require(turn<=glm::radians(660.f)*dt+.0005f,"flanking and planting the feet turn continuously");
                const auto displacement=dog.getPosition()-before;
                if(t<3.f) {
                    distanceTravelled+=glm::length(displacement);
                    if(glm::length(displacement)>.001f)
                        require(glm::dot(glm::normalize(displacement),heading)>.94f,"flank body faces its actual travel");
                }
                warned|=countActions(dog,EnemyAction::Kind::Windup)>0;
                previousHeading=heading;
            }
            require(distanceTravelled>20.f,"hound stays mobile while waiting for a heavy slot");
            require(warned,"circling hound can commit to a dash once permitted");
        }
    }
}

void latePursuit() {
    const auto map=floorMap();
    const glm::vec3 player(0,.05f,0);
    const AABB bounds(player+glm::vec3(-.3f,0,-.3f),player+glm::vec3(.3f,1.8f,.3f));
    Enemy direct({0,.05f,-14},6,EnemyType::Chaser);
    Enemy flank=direct;
    direct.setId(3);flank.setId(2);
    for(Enemy* actor:{&direct,&flank}) {
        actor->setPressureStage(4);
        actor->setAttackContext(true,{7,0,0},bounds);
        actor->setArenaEnabled(true);
        actor->update(.1f,player,&map);
    }
    require(direct.getPosition().x==0 && flank.getPosition().x>.3f,
            "late packs split direct and interception lanes around a moving target");
    const glm::vec3 close=flank.getPosition()+glm::vec3(0,0,1);
    require(flank.isTouchingPlayer(close) && !flank.isTouchingPlayer(close+glm::vec3(0,0,1)),
            "late contact still requires reaching the same physical melee radius");
    auto crystal=std::make_shared<ArenaDefinition>();
    crystal->encounterProfile=EncounterProfile::CrystalSurvival;
    EnemyManager crowd;
    crowd.setArenaDefinition(crystal);
    for(int i=0;i<10;++i)
        require(crowd.spawn(EnemyType::Chaser,{}),"ten Crystal chasers can coexist");
    for(unsigned stage=0;stage<=4;++stage) {
        crowd.setPressureStage(stage);
        require(crowd.count(EnemyType::Chaser)==10 && !crowd.spawn(EnemyType::Chaser,{}),
                "the direct spawn path preserves survivors and rejects an eleventh chaser in every stage");
    }
    EnemyManager classic;
    classic.setArenaDefinition(std::make_shared<ArenaDefinition>(ArenaDefinition::starryCrystal()));
    for(int i=0;i<18;++i)
        require(classic.spawn(EnemyType::Chaser,{}),"classic arena still admits eighteen chasers");
    require(!classic.spawn(EnemyType::Chaser,{}),"classic chaser cap remains eighteen");
}

void sharedHeavySchedule() {
    EnemyManager enemies;
    EnemyProjectileSystem shots;
    const auto floor=floorMap();
    const glm::vec3 player(0);
    const AABB bounds({-.3f,0,-.3f},{.3f,1.8f,.3f});
    const auto warnings=[&]() {
        return std::count_if(enemies.actions().begin(),enemies.actions().end(),
            [](const EnemyAction& action){return action.kind==EnemyAction::Kind::Windup;});
    };
    const auto tick=[&](float dt) {enemies.update(dt,player,&floor,&bounds,shots.activeVolleyCount());};
    require(enemies.spawn(EnemyType::VeilRay,{0,5,-7}) &&
            enemies.spawn(EnemyType::HangingCenser,{20,6,0}),"spawn competing heavy attackers");
    shots.spawnVolley({0,6,-10},{0,.04f,0},20);
    shots.update(.6f,&floor,bounds);
    shots.spawnVolley({0,6,-10},{0,.04f,0},21);
    tick(.42f);
    require(warnings()==0 && shots.activeVolleyCount()==2,"two live volleys occupy both heavy slots");
    shots.update(.42f,&floor,bounds);
    require(shots.activeVolleyCount()==1,"first airborne volley frees one slot on impact");
    tick(.01f);
    require(warnings()==1 && enemies.getEnemies()[0].getAttackPhase()==AttackPhase::Windup &&
            enemies.getEnemies()[1].getAttackPhase()==AttackPhase::Approach,
            "only first ready attacker can enter the available slot on a simultaneous request");
    shots.update(.7f,&floor,bounds);
    tick(0);
    require(warnings()==0,"paused manager produces no actions");
    tick(1.49f);
    require(warnings()==0,"free slots cannot shorten the 1.5-second warning spacing");
    tick(.011f);
    require(warnings()==1 && enemies.getEnemies()[1].getAttackPhase()==AttackPhase::Windup,
            "second eligible attacker starts immediately after spacing elapses");
    enemies.clear();shots.clear();
    require(enemies.actions().empty() && enemies.getLiveCount()==0,"restart removes attackers and warning list");
    enemies.spawn(EnemyType::VeilRay,{0,5,-7});
    enemies.spawn(EnemyType::HangingCenser,{20,6,0});
    tick(.01f);
    require(warnings()==1 && enemies.getEnemies()[0].getAttackPhase()==AttackPhase::Windup &&
            enemies.getEnemies()[1].getAttackPhase()==AttackPhase::Approach,
            "restart clears cooldown and counts while same-frame arbitration still permits one start");
}

void censerGlandAndBurst() {
    auto map=floorMap();
    Enemy censer({0,6,-20},24,EnemyType::HangingCenser);
    const glm::vec3 player(0,0,0);
    censer.update(.05f,player,&map);
    require(censer.getAttackPhase()==AttackPhase::Windup,"censer opens in range and sight");
    const auto mouth=crystalCombat::CenserMouthOffset;
    requireNear(censer.getDimensions().y,8.4f,.001f,"censer shell is 8.4 metres tall");
    const glm::vec3 core=censer.getPosition()+glm::vec3(0,mouth.y,0)+censer.getHeading()*mouth.z;
    bool gland=false;float t=0;
    require(censer.intersectShot({0,1.65f,0},core,t,gland) && gland,"ADS line from ground sees exposed core");
    bool offsetHit=false;
    require(censer.intersectShot({.52f,1.65f,0},core+glm::vec3(.52f,0,0),t,offsetHit) && offsetHit,
            "enlarged exposed sac accepts an off-centre shot outside its old radius");
    bool behind=true;
    require(censer.intersectShot(core-censer.getHeading()*4.f,core,t,behind) && !behind,"back shell conceals weakpoint");
    censer.receiveShot(1,gland);censer.receiveShot(1,gland);
    require(censer.getInterruptHits()==2 && censer.getAttackPhase()==AttackPhase::Windup,"two hits leave window open");
    auto released=censer;
    released.update(1.4f,player,&map);
    require(countActions(released,EnemyAction::Kind::Release)==1,"two core hits do not cancel release");
    for(const auto& a:released.actions()) if(a.kind==EnemyAction::Kind::Release)
        require(glm::distance(a.position,core)<.001f,"crystals emerge from the scaled mouth");
    censer.receiveShot(1,true);
    censer.update(.35f,player,&map);
    require(countActions(censer,EnemyAction::Kind::Interrupt)==1 && countActions(censer,EnemyAction::Kind::Release)==0,
            "third gland hit cancels volley exactly once and body drops");
    const auto lowered=censer.getPosition();
    requireNear(lowered.y,5.f,.01f,"interrupted censer drops one metre from its actual windup height");
    censer.update(.8f,player,&map);
    require(censer.getAttackPhase()==AttackPhase::Interrupted && glm::distance(lowered,censer.getPosition())<.001f,
            "censer holds its dropped position for the interrupt stun");
    censer.update(1.5f,player,&map);
    const auto raised=censer.getPosition();
    require(censer.getAttackPhase()==AttackPhase::Recovery && raised.y>lowered.y+.65f &&
            glm::length(glm::vec2(raised.x-lowered.x,raised.z-lowered.z))>1.f,
            "recovery restores altitude during ordinary lateral drift");
    censer.update(1.4f,player,&map);
    require(countActions(censer,EnemyAction::Kind::Windup)==0,"three-second cooldown prevents early reopening");
    censer.update(.3f,player,&map);
    require(censer.getAttackPhase()==AttackPhase::Windup,"censer can reopen when drift cooldown has elapsed");
    const auto fixed=censer.getPosition();
    censer.update(.4f,player,&map);
    require(glm::distance(fixed,censer.getPosition())<.001f,"open sac stays steady under ADS");
    Enemy waiting({0,6,-20},24,EnemyType::HangingCenser);
    waiting.setAttackContext(false,{},AABB({-.3f,0,-.3f},{.3f,1.8f,.3f}));
    float low=6,high=6;
    for(int frame=0;frame<300;++frame) {
        waiting.update(1.f/60,player,&map);
        low=std::min(low,waiting.getPosition().y);high=std::max(high,waiting.getPosition().y);
    }
    require(high-low>.28f && waiting.getAttackPhase()==AttackPhase::Approach,
            "waiting censer bobs at its real collision root while a heavy slot is unavailable");
    auto perch=floorMap();
    require(perch.addBrush(box({-.7f,0,-20.7f},{.7f,5.6f,-19.3f})),"add cover beneath hovering censer");
    Enemy overCover({0,6,-20},24,EnemyType::HangingCenser);
    overCover.update(.05f,player,&perch);
    for(int shot=0;shot<3;++shot) overCover.receiveShot(1,true);
    overCover.update(.65f,player,&perch);
    require(overCover.getAttackPhase()==AttackPhase::Interrupted && overCover.getPosition().y>=5.55f &&
            overCover.getPosition().y<5.8f,"interrupted drop stops on solid cover instead of passing through it");
    auto dead=released;dead.takeDamage(100);dead.update(8,player,&map);
    require(dead.actions().empty(),"dead censer emits no new action");

    EnemyProjectileSystem shots;
    const AABB body({-.3f,0,-.3f},{.3f,1.8f,.3f});
    require(shots.spawnVolley(core,{0,.04f,0},127),"all three crystals are admitted together");
    require(shots.size()==3,"volley has three visible projectiles");
    const auto frozen=shots.projectiles()[0].position;
    shots.update(0,&map,body);
    require(glm::length(frozen-shots.projectiles()[0].position)==0,"volley freezes at pause");
    float damage=0;
    for(auto& hit:shots.update(1.1f,&map,body)) {
        require(hit.sourceId==127 && hit.source==DamageSource::CenserBurst,"volley preserves owner and damage source");
        damage+=hit.damage;
    }
    requireNear(damage,15,.001f,"one volley capped at fifteen actual damage");
    require(shots.size()==0,"all expired after impact");
    require(map.addBrush(box({-2,0,-1.9f},{2,15,-1.4f})),"valid cover fixture");
    shots.spawnVolley(core,{0,.04f,0},127);
    require(shots.update(1.1f,&map,body).empty(),"solid cover blocks incoming volley and splash");
    shots.spawnVolley(core,{0,.04f,0},127);shots.clear();
    require(shots.size()==0,"restart clears pending volley and its group");
}

}
void runEnemyCombatTests() {
    crystalAttacks(); scaledFlyingBodies(); lockedHoundAndBlockedLane(); houndFlankFacing(); latePursuit(); sharedHeavySchedule(); censerGlandAndBurst();
    sentryTiming(); sentryVisibility(); sentryFrameRates(); projectileSweeps();
    chaserNavigation(); chaserEdgeNavigation(); chaserCoverTopNavigation();
}
