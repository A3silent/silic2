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
    enum class Kind { Charge, Fire, CancelCharge, Windup, Release, Strike, Interrupt };
    Kind kind;
    uint32_t sourceId;
    glm::vec3 position, direction;
    EnemyType type=EnemyType::Sentry;
    glm::vec3 target{};
    float damage=0;
};

class Enemy {
public:
    Enemy(const glm::vec3& spawnPosition, int maxHp = 3,
          EnemyType type = EnemyType::Chaser);

    void update(float deltaTime, const glm::vec3& playerPos, const Map* map);
    void takeDamage(int amount);
    // All three new enemies use local +Z forward and a bottom-center origin.
    glm::vec3 getDimensions() const;
    AttackPhase getAttackPhase() const { return phase; }
    float getPhaseProgress() const;
    glm::vec3 getLockedTarget() const { return lockedTarget; }
    glm::vec3 getSweepStart() const { return sweepStart; }
    glm::vec3 getSweepEnd() const { return sweepEnd; }
    int getInterruptHits() const { return glandHits; }
    bool intersectShot(glm::vec3 start,glm::vec3 end,float& fraction,bool& gland) const;
    void receiveShot(int amount,bool gland);
    void setAttackContext(bool permit,glm::vec3 velocity,const AABB& player) {
        attackPermit=permit; playerVelocity=velocity; playerBounds=player; hasContext=true;
    }
    // Compatibility aggro toggle; map navigation is supplied explicitly.
    void setArenaEnabled(bool enabled) { arenaEnabled=enabled; }
    void setArenaDefinition(std::shared_ptr<const ArenaDefinition> definition) {
        arenaDefinition=std::move(definition); arenaEnabled=bool(arenaDefinition);
    }
    void setId(uint32_t value) { id=value; }
    void setSeparation(glm::vec3 value) { separation=value; }
    void setPressureStage(unsigned value) { pressureStage=std::min(value,4u); }
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
    AttackPhase phase=AttackPhase::Approach;
    float phaseTime=0, phaseLength=0, approachTime=0, flightAltitude=0;
    float driftTime=0, interruptHeight=0;
    glm::vec3 lockedTarget{}, sweepStart{}, sweepEnd{}, playerVelocity{};
    glm::vec3 houndFlankTarget{};
    bool hasFlankTarget=false;
    float rayReplanTime=0;
    bool hasRayLane=false;
    AABB playerBounds;
    int glandHits=0, nextSegment=0;
    bool attackPermit=true, damageDelivered=false, reportInterrupt=false, hasContext=false;
    void updateCrystal(float dt,glm::vec3 player,const Map* map);
    void advanceCrystal(float dt,glm::vec3 player,const Map* map);
    void approachCrystal(float dt,glm::vec3 player,const Map* map);
    void updateWindup(float dt,glm::vec3 player,const Map* map);
    void updateStrike(float dt,const Map* map);
    void setPhase(AttackPhase next,float duration=0);
    void crystalEvent(EnemyAction::Kind event,float damage=0);
    bool flyToward(glm::vec3 target,float distance,const Map* map);
    void driftCenser(float dt,glm::vec3 player,const Map* map);
    void moveHound(glm::vec3 target,float dt,const Map* map);
    glm::vec3 findHoundEntry(glm::vec3 player,glm::vec3 forward,float flank,const Map* map) const;
    glm::vec3 position;
    glm::vec3 heading{0,0,-1}, separation{0}, lockedDirection{0,0,-1};
    uint32_t id=0;
    unsigned pressureStage=0;
    bool arenaEnabled=false;
    std::shared_ptr<const ArenaDefinition> arenaDefinition;
    float chargeTime=0, recoverTime=0, secondsSinceDamage=10000;
    float movementSpeed=0;
    float navigationRefresh=0;
    glm::vec3 navigationTarget{0};
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
