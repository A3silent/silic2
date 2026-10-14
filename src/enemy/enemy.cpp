#include "enemy/enemy.h"
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
    const float half=type==EnemyType::Sentry ? 0.6f : HALF_W;
    const float height=type==EnemyType::Sentry ? 1.0f : BOX_HEIGHT;
    return AABB(position-glm::vec3(half,0,half),position+glm::vec3(half,height,half));
}

bool Enemy::isTouchingPlayer(const glm::vec3& playerPos) const {
    if (isDead() || type==EnemyType::Sentry) return false;
    if(arenaDefinition && arenaDefinition->spatialWorld) {
        if(std::abs(position.y-playerPos.y)>1.4f)return false;
        if(arenaDefinition->spatialWorld->raycast(position+glm::vec3(0,.9f,0),playerPos+glm::vec3(0,.9f,0)).hit)return false;
    }
    float dx = position.x - playerPos.x;
    float dz = position.z - playerPos.z;
    return std::sqrt(dx * dx + dz * dz) < TOUCH_RANGE;
}

void Enemy::update(float deltaTime, const glm::vec3& playerPos, const Map* map) {
    pendingActions.clear();
    if (isDead() || deltaTime<=0) return;
    movementSpeed=0;
    const auto previousPosition=position;
    secondsSinceDamage+=deltaTime;
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
    glm::vec3 target=playerPos;
    if(arenaEnabled && arenaDefinition) {
        if(arenaDefinition->spatialWorld) {
            navigationRefresh-=deltaTime;
            if(navigationRefresh<=0 || glm::length(glm::vec2(position.x-navigationTarget.x,position.z-navigationTarget.z))<.3f) {
                navigationTarget=arenaDefinition->nextWaypoint(position,playerPos,HALF_W+0.04f);
                navigationRefresh=.18f+float(id%6)*.02f;
            }
            target=navigationTarget;
        } else target=arenaDefinition->nextWaypoint(position,playerPos,HALF_W+0.04f);
    }
    glm::vec3 toPlayer(target.x-position.x,0,target.z-position.z);
    float dist=glm::length(toPlayer);
    if(dist<0.01f) return;
    heading=toPlayer/dist;
    glm::vec3 velocity=heading*MOVE_SPEED+separation;
    if(glm::length(velocity)>MOVE_SPEED) velocity=glm::normalize(velocity)*MOVE_SPEED;
    glm::vec3 desired=velocity*std::min(deltaTime,dist/MOVE_SPEED);

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
