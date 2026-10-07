#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <algorithm>
#include <vector>
#include <memory>
#include "engine/collision.h"
#include "game/gameplay_types.h"

namespace silic2 {

class Map;
struct ArenaDefinition;

enum class EnemyState {
    IDLE,
    CHASING,
    DEAD,
    CHARGING,
    RECOVERING
};

struct EnemyAction {
    enum class Kind { Charge, Fire, CancelCharge };
    Kind kind;
    uint32_t sourceId;
    glm::vec3 position, direction;
};

class Enemy {
public:
    Enemy(const glm::vec3& spawnPosition, int maxHp = 3,
          EnemyType type = EnemyType::Chaser);

    void update(float deltaTime, const glm::vec3& playerPos, const Map* map);
    void takeDamage(int amount);
    // Compatibility aggro toggle; map navigation is supplied explicitly.
    void setArenaEnabled(bool enabled) { arenaEnabled=enabled; }
    void setArenaDefinition(std::shared_ptr<const ArenaDefinition> definition) {
        arenaDefinition=std::move(definition); arenaEnabled=bool(arenaDefinition);
    }
    void setId(uint32_t value) { id=value; }
    void setSeparation(glm::vec3 value) { separation=value; }
    uint32_t getId() const { return id; }
    glm::vec3 getHeading() const { return heading; }
    float getMovementSpeed() const { return movementSpeed; }
    float getChargeProgress() const { return state==EnemyState::CHARGING ? chargeTime : 0; }
    bool isCharging() const { return state==EnemyState::CHARGING; }
    bool isAimLocked() const { return isCharging() && chargeTime>=0.65f; }
    float getHitFlash() const { return std::max(0.0f,1.0f-secondsSinceDamage/0.14f); }
    float getSecondsSinceDamage() const { return secondsSinceDamage; }
    const std::vector<EnemyAction>& actions() const { return pendingActions; }

    bool isDead()        const { return currentHp <= 0; }
    glm::vec3 getPosition() const { return position; }
    EnemyState getState()   const { return state; }
    int getHp()          const { return currentHp; }
    int getMaxHp()       const { return maxHp; }
    EnemyType getType()  const { return type; }

    AABB getAABB() const;

    // True when this enemy is close enough to melee the player
    bool isTouchingPlayer(const glm::vec3& playerPos) const;

    // Box dimensions (used by renderer for model matrix scaling)
    static constexpr float BOX_WIDTH  = 0.6f;
    static constexpr float BOX_HEIGHT = 1.8f;

private:
    glm::vec3 position;
    glm::vec3 heading{0,0,-1}, separation{0}, lockedDirection{0,0,-1};
    uint32_t id=0;
    bool arenaEnabled=false;
    std::shared_ptr<const ArenaDefinition> arenaDefinition;
    float chargeTime=0, recoverTime=0, secondsSinceDamage=10000;
    float movementSpeed=0;
    std::vector<EnemyAction> pendingActions;
    void updateSentry(float deltaTime,const glm::vec3& playerPos,const Map* map);
    float     velocityY = 0.0f; // vertical velocity (gravity)
    bool      onGround  = false;

    int currentHp;
    int maxHp;
    EnemyType type;
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
