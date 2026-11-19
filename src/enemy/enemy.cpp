#include "enemy/enemy.h"
#include "enemy/crystal_combat.h"
#include "engine/map.h"
#include "enemy/enemy_geometry.h"
#include "game/arena_definition.h"
#include "engine/collision.h"
#include <algorithm>
#include <cmath>

namespace silic2 {

Enemy::Enemy(const glm::vec3& spawnPosition, int hp, EnemyType enemyType)
    : position(spawnPosition), currentHp(hp), maxHp(hp), type(enemyType),
      state(EnemyState::IDLE)
{
}

AABB Enemy::getAABB() const {
    auto size=getDimensions();
    glm::vec3 half(size.x*.5f,0,size.z*.5f);
    if(crystalCombat::isCrystalEnemy(type)) {
        const float hx=half.x, hz=half.z;
        half.x=hx*std::abs(heading.z)+hz*std::abs(heading.x);
        half.z=hx*std::abs(heading.x)+hz*std::abs(heading.z);
    }
    const float wingMargin=type==EnemyType::VeilRay ? .85f*crystalCombat::bodyScale(type) : 0.f;
    return {position-half-glm::vec3(0,wingMargin,0),
            position+half+glm::vec3(0,size.y+wingMargin,0)};
}

bool Enemy::isTouchingPlayer(const glm::vec3& playerPos) const {
    if (isDead() || type!=EnemyType::Chaser) return false;
    const glm::vec2 horizontal(position.x-playerPos.x,position.z-playerPos.z);
    if(glm::dot(horizontal,horizontal)>=TOUCH_RANGE*TOUCH_RANGE) return false;
    if(arenaDefinition && arenaDefinition->spatialWorld) {
        if(std::abs(position.y-playerPos.y)>1.4f)return false;
        if(arenaDefinition->spatialWorld->raycast(position+glm::vec3(0,.9f,0),playerPos+glm::vec3(0,.9f,0)).hit)return false;
    }
    return true;
}

void Enemy::update(float deltaTime, const glm::vec3& playerPos, const Map* map) {
    pendingActions.clear();
    if (isDead() || deltaTime<=0) return;
    movementSpeed=0;
    const auto previousPosition=position;
    secondsSinceDamage+=deltaTime;
    if(crystalCombat::isCrystalEnemy(type)) {
        updateCrystal(deltaTime,playerPos,map);
        movementSpeed=glm::length(position-previousPosition)/deltaTime;
        return;
    }
    applyGravityAndGround(deltaTime, map);
    if(type==EnemyType::Sentry) { updateSentry(deltaTime,playerPos,map); return; }

    float dx = playerPos.x - position.x;
    float dz = playerPos.z - position.z;
    float dist = std::sqrt(dx * dx + dz * dz);

    if (arenaEnabled || dist < AGGRO_RANGE) {
        state = EnemyState::CHASING;
        moveTowardPlayer(deltaTime, playerPos, map);
    } else {
        state = EnemyState::IDLE;
    }
    movementSpeed=glm::length(glm::vec2(position.x-previousPosition.x,position.z-previousPosition.z))/deltaTime;
}

void Enemy::updateSentry(float dt,const glm::vec3& playerPos,const Map* map) {
    const glm::vec3 target=playerPos+glm::vec3(0,0.9f,0);
    const glm::vec3 muzzle=position+glm::vec3(0,0.65f,0);
    auto directionToPlayer=[&]() {
        const auto offset=target-muzzle;
        return glm::length(offset)>1e-6f ? glm::normalize(offset) : heading;
    };
    // Consume phase boundaries exactly even on irregular simulation steps.
    float remaining=dt;
    for(int boundary=0;boundary<32 && remaining>0; ++boundary) {
        if(state==EnemyState::IDLE) {
            heading=directionToPlayer();
            if(!enemyGeometry::clear(muzzle,target,map,0.12f)) break;
            state=EnemyState::CHARGING; chargeTime=0; lockedDirection=heading;
            pendingActions.push_back({EnemyAction::Kind::Charge,id,muzzle,heading});
        }
        if(state==EnemyState::CHARGING) {
            if(chargeTime<0.65f) {
                heading=directionToPlayer(); lockedDirection=heading;
                const float step=std::min(remaining,0.65f-chargeTime);
                chargeTime+=step; remaining-=step;
                if(chargeTime<0.65f) break;
            }
            const float step=std::min(remaining,1.0f-chargeTime);
            chargeTime+=step; remaining-=step;
            if(chargeTime<1.0f) break;
            // Recheck the locked ray through the current target distance. A moving
            // player does not retarget a locked shot; cover can cancel the shot.
            const float range=std::max(1.0f,glm::length(target-muzzle));
            if(enemyGeometry::clear(muzzle,muzzle+lockedDirection*range,map,0.12f))
                pendingActions.push_back({EnemyAction::Kind::Fire,id,muzzle,lockedDirection});
            else pendingActions.push_back({EnemyAction::Kind::CancelCharge,id,muzzle,lockedDirection});
            state=EnemyState::RECOVERING; recoverTime=2.2f; chargeTime=0;
        }
        if(state==EnemyState::RECOVERING) {
            heading=directionToPlayer();
            const float step=std::min(remaining,recoverTime);
            recoverTime-=step; remaining-=step;
            if(recoverTime>0) break;
            state=EnemyState::IDLE;
        }
    }
}

void Enemy::takeDamage(int amount) {
    if (isDead() || amount<=0) return;
    secondsSinceDamage=0;
    currentHp -= amount;
    if (currentHp <= 0) {
        currentHp = 0;
        state = EnemyState::DEAD;
    }
}

void Enemy::applyGravityAndGround(float deltaTime, const Map* map) {
    const float halfWidth=getAABB().getSize().x*0.5f;
    if(map && map->getSpatialWorld()) {
        auto world=map->getSpatialWorld();auto ground=world->support(position,.08f,.12f,halfWidth);
        onGround=ground.hit && position.y>=ground.height-.06f && position.y-ground.height<=.12f;
        if(onGround){position.y=ground.height+.015f;velocityY=0;return;}
        velocityY=std::max(velocityY+GRAVITY*deltaTime,MAX_FALL);
        auto move=world->moveActor(position,{0,velocityY*deltaTime,0},halfWidth,getAABB().getSize().y,false);
        position=move.position;onGround=move.grounded;if(onGround||move.ceiling)velocityY=0;return;
    }
    onGround = checkGroundBelow(map);

    if (onGround) {
        velocityY = 0.0f;
        return;
    }

    velocityY = std::max(velocityY + GRAVITY * deltaTime, MAX_FALL);
    const float previousFeetY = position.y;
    const float nextFeetY = position.y + velocityY * deltaTime;

    // Sweep the feet through the full vertical step. A thin point probe can
    // skip zero-thickness floors when frame time varies.
    bool foundLanding = false;
    float landingY = 0.0f;
    if (map && velocityY <= 0.0f) {
        for (const auto& brush : map->getBrushes()) {
            if (brush.vertices.empty()) continue;

            glm::vec3 brushMin = brush.vertices[0];
            glm::vec3 brushMax = brush.vertices[0];
            for (const auto& vertex : brush.vertices) {
                brushMin = glm::min(brushMin, vertex);
                brushMax = glm::max(brushMax, vertex);
            }

            const bool overlapsX =
                position.x + halfWidth >= brushMin.x &&
                position.x - halfWidth <= brushMax.x;
            const bool overlapsZ =
                position.z + halfWidth >= brushMin.z &&
                position.z - halfWidth <= brushMax.z;
            const float surfaceY = brushMax.y;
            const bool crossedSurface =
                surfaceY <= previousFeetY + GROUND_EPS &&
                surfaceY >= nextFeetY - GROUND_EPS;

            if (overlapsX && overlapsZ && crossedSurface &&
                (!foundLanding || surfaceY > landingY)) {
                foundLanding = true;
                landingY = surfaceY;
            }
        }
    }

    if (foundLanding) {
        position.y = landingY + GROUND_EPS;
        velocityY = 0.0f;
        onGround = true;
    } else {
        position.y = nextFeetY;
    }
}

bool Enemy::checkGroundBelow(const Map* map) const {
    if (!map) return false;

    const float halfWidth=getAABB().getSize().x*0.5f;
    AABB groundProbe(
        glm::vec3(position.x - halfWidth, position.y - GROUND_EPS, position.z - halfWidth),
        glm::vec3(position.x + halfWidth, position.y,               position.z + halfWidth)
    );

    for (const auto& brush : map->getBrushes()) {
        if (brush.vertices.empty()) continue;
        glm::vec3 bMin = brush.vertices[0], bMax = brush.vertices[0];
        for (const auto& v : brush.vertices) {
            bMin = glm::min(bMin, v);
            bMax = glm::max(bMax, v);
        }
        AABB brushBox(bMin, bMax);
        if (CollisionSystem::checkAABB(groundProbe, brushBox) && bMax.y <= position.y + 0.1f) {
            return true;
        }
    }
    return false;
}

void Enemy::moveTowardPlayer(float deltaTime, const glm::vec3& playerPos, const Map* map) {
    constexpr float pursuitSpeed[]={MOVE_SPEED,MOVE_SPEED,4.4f,6.3f,9.3f};
    const float speed=pursuitSpeed[pressureStage];
    glm::vec3 target=playerPos;
    if(pressureStage>=2) {
        auto offset=playerPos-position;offset.y=0;
        // A third of the pack follows directly. Flanking packs aim ahead,
        // converging on the real player at close range so a reversal beats the lead.
        const float lead=std::min(glm::length(offset)/19.f,1.f)*float(id%3)*.8f;
        target+=playerVelocity*lead;
    }
    if(arenaEnabled && arenaDefinition) {
        if(arenaDefinition->spatialWorld) {
            navigationRefresh-=deltaTime;
            if(navigationRefresh<=0 || glm::length(glm::vec2(position.x-navigationTarget.x,position.z-navigationTarget.z))<.3f) {
                const auto& world=*arenaDefinition->spatialWorld;
                const glm::vec3 eye(0,.9f,0);
                const glm::vec3 delta=target-position;
                const float reach=std::min(1.f,(speed*.42f+.5f)/std::max(.01f,glm::length(delta)));
                const auto ahead=position+delta*reach;
                // Verify only the next short section of a clear sightline. A
                // distant squad need not sample the whole floor every replan.
                if(pressureStage && !world.raycast(position+eye,target+eye,HALF_W+.04f).hit &&
                   world.canTraverse(position,ahead,HALF_W+.04f)) navigationTarget=ahead;
                else navigationTarget=arenaDefinition->nextWaypoint(position,playerPos,HALF_W+.04f);
                navigationRefresh=.18f+float(id%6)*.02f;
            }
            target=navigationTarget;
        } else target=arenaDefinition->nextWaypoint(position,target,HALF_W+0.04f);
    }
    glm::vec3 toPlayer(target.x-position.x,0,target.z-position.z);
    float dist=glm::length(toPlayer);
    if(dist<0.01f) return;
    heading=toPlayer/dist;
    glm::vec3 velocity=heading*speed+separation;
    if(glm::length(velocity)>speed) velocity=glm::normalize(velocity)*speed;
    glm::vec3 desired=velocity*std::min(deltaTime,dist/speed);

    if(map && map->getSpatialWorld()) {
        auto move=map->getSpatialWorld()->moveActor(position,desired,HALF_W,BOX_HEIGHT,onGround);
        position=move.position;onGround=move.grounded;return;
    }
    glm::vec3 actual = resolveHorizontalCollision(desired, map);
    position.x += actual.x;
    position.z += actual.z;
}

glm::vec3 Enemy::resolveHorizontalCollision(const glm::vec3& desiredMove, const Map* map) const {
    if (!map) return desiredMove;

    glm::vec3 result = desiredMove;
    AABB current = getAABB();

    // Sweep each horizontal axis to avoid tunneling at irregular frame steps.
    for (int axis : {0,2}) {
        glm::vec3 axisMove(0); axisMove[axis]=result[axis];
        const auto center=current.getCenter();
        const auto half=current.getSize()*0.5f;
        float allowed=1;
        for(const auto& brush:map->getBrushes()) {
            if(brush.vertices.empty()) continue;
            auto box=enemyGeometry::bounds(brush);
            if(box.max.y<=current.min.y+0.01f || box.min.y>=current.max.y) continue;
            box.min-=half; box.max+=half;
            float t;
            if(enemyGeometry::segment(center,center+axisMove,box,t)) allowed=std::min(allowed,std::max(0.0f,t-0.001f));
        }
        result[axis]*=allowed;
        axisMove[axis]=result[axis];
        current.min+=axisMove; current.max+=axisMove;
    }

    return result;
}

} // namespace silic2
