#pragma once

#include <glm/glm.hpp>
#include "engine/collision.h"

namespace silic2 {

class Map;

enum class EnemyState {
    IDLE,
    CHASING,
    DEAD
};

class Enemy {
public:
    Enemy(const glm::vec3& spawnPosition, int maxHp = 3);

    void update(float deltaTime, const glm::vec3& playerPos, const Map* map);
    void takeDamage(int amount);

    bool isDead()        const { return currentHp <= 0; }
    glm::vec3 getPosition() const { return position; }
    EnemyState getState()   const { return state; }
    int getHp()          const { return currentHp; }
    int getMaxHp()       const { return maxHp; }

    AABB getAABB() const;

    // True when this enemy is close enough to melee the player
    bool isTouchingPlayer(const glm::vec3& playerPos) const;

    // Box dimensions (used by renderer for model matrix scaling)
    static constexpr float BOX_WIDTH  = 0.6f;
    static constexpr float BOX_HEIGHT = 1.8f;

private:
    glm::vec3 position;
    float     velocityY = 0.0f; // vertical velocity (gravity)
    bool      onGround  = false;

    int currentHp;
    int maxHp;
    EnemyState state;

    static constexpr float HALF_W      = BOX_WIDTH  * 0.5f;
    static constexpr float MOVE_SPEED  = 3.0f;   // units/s horizontal
    static constexpr float AGGRO_RANGE = 15.0f;  // player detection radius
    static constexpr float TOUCH_RANGE = 1.2f;   // melee contact radius (horizontal)
    static constexpr float GRAVITY     = -20.0f;
    static constexpr float MAX_FALL    = -50.0f;
    static constexpr float GROUND_EPS  = 0.05f;  // ground check extension

    void applyGravityAndGround(float deltaTime, const Map* map);
    void moveTowardPlayer(float deltaTime, const glm::vec3& playerPos, const Map* map);

    // Returns the actual movement after wall collision resolution (horizontal only)
    glm::vec3 resolveHorizontalCollision(const glm::vec3& desiredMove, const Map* map) const;

    bool checkGroundBelow(const Map* map) const;
};

} // namespace silic2
