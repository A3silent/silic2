#include "enemy/enemy.h"
#include "engine/map.h"
#include "engine/collision.h"
#include <algorithm>
#include <cmath>

namespace silic2 {

Enemy::Enemy(const glm::vec3& spawnPosition, int hp)
    : position(spawnPosition), currentHp(hp), maxHp(hp), state(EnemyState::IDLE)
{
}

AABB Enemy::getAABB() const {
    return AABB(
        glm::vec3(position.x - HALF_W, position.y,            position.z - HALF_W),
        glm::vec3(position.x + HALF_W, position.y + BOX_HEIGHT, position.z + HALF_W)
    );
}

bool Enemy::isTouchingPlayer(const glm::vec3& playerPos) const {
    if (isDead()) return false;
    float dx = position.x - playerPos.x;
    float dz = position.z - playerPos.z;
    return std::sqrt(dx * dx + dz * dz) < TOUCH_RANGE;
}

void Enemy::update(float deltaTime, const glm::vec3& playerPos, const Map* map) {
    if (isDead()) return;

    applyGravityAndGround(deltaTime, map);

    float dx = playerPos.x - position.x;
    float dz = playerPos.z - position.z;
    float dist = std::sqrt(dx * dx + dz * dz);

    if (dist < AGGRO_RANGE) {
        state = EnemyState::CHASING;
        moveTowardPlayer(deltaTime, playerPos, map);
    } else {
        state = EnemyState::IDLE;
    }
}

void Enemy::takeDamage(int amount) {
    if (isDead()) return;
    currentHp -= amount;
    if (currentHp <= 0) {
        currentHp = 0;
        state = EnemyState::DEAD;
    }
}

void Enemy::applyGravityAndGround(float deltaTime, const Map* map) {
    onGround = checkGroundBelow(map);

    if (!onGround) {
        velocityY += GRAVITY * deltaTime;
        velocityY = std::max(velocityY, MAX_FALL);
    } else {
        velocityY = 0.0f;
    }

    float yMove = velocityY * deltaTime;
    position.y += yMove;

    // Re-check ground after moving; snap if we landed
    if (checkGroundBelow(map) && velocityY <= 0.0f) {
        onGround = true;
        velocityY = 0.0f;

        // Snap feet to the top of the highest brush below
        if (map) {
            float highestTop = 0.0f;
            AABB groundProbe(
                glm::vec3(position.x - HALF_W, position.y - GROUND_EPS, position.z - HALF_W),
                glm::vec3(position.x + HALF_W, position.y,               position.z + HALF_W)
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
                    highestTop = std::max(highestTop, bMax.y);
                }
            }
            position.y = highestTop + GROUND_EPS;
        }
    }
}

bool Enemy::checkGroundBelow(const Map* map) const {
    if (!map) return false;

    AABB groundProbe(
        glm::vec3(position.x - HALF_W, position.y - GROUND_EPS, position.z - HALF_W),
        glm::vec3(position.x + HALF_W, position.y,               position.z + HALF_W)
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
    glm::vec3 toPlayer(playerPos.x - position.x, 0.0f, playerPos.z - position.z);
    float dist = glm::length(toPlayer);
    if (dist < 0.01f) return;

    glm::vec3 dir = toPlayer / dist;
    glm::vec3 desired = dir * MOVE_SPEED * deltaTime;

    glm::vec3 actual = resolveHorizontalCollision(desired, map);
    position.x += actual.x;
    position.z += actual.z;
}

glm::vec3 Enemy::resolveHorizontalCollision(const glm::vec3& desiredMove, const Map* map) const {
    if (!map) return desiredMove;

    glm::vec3 result = desiredMove;
    AABB current = getAABB();

    // Axis-separated resolution: try X, then Z independently
    for (int axis = 0; axis < 2; ++axis) {
        glm::vec3 axisMove(0.0f);
        if (axis == 0) axisMove.x = result.x;
        else           axisMove.z = result.z;

        AABB moved(current.min + axisMove, current.max + axisMove);

        for (const auto& brush : map->getBrushes()) {
            if (brush.vertices.empty()) continue;
            glm::vec3 bMin = brush.vertices[0], bMax = brush.vertices[0];
            for (const auto& v : brush.vertices) {
                bMin = glm::min(bMin, v);
                bMax = glm::max(bMax, v);
            }
            // Skip zero-thickness horizontal brushes (floors/ceilings) — not walls
            if ((bMax.y - bMin.y) < 0.01f) continue;
            AABB brushBox(bMin, bMax);
            if (CollisionSystem::checkAABB(moved, brushBox)) {
                if (axis == 0) result.x = 0.0f;
                else           result.z = 0.0f;
                break;
            }
        }
    }

    return result;
}

} // namespace silic2
