#pragma once

#include <vector>
#include <memory>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "enemy/enemy.h"
#include "engine/map.h"
#include "engine/map_renderer.h"

namespace silic2 {

class Shader;

class EnemyManager {
public:
    struct BulletHitResult {
        bool hit = false;
        bool killed = false;
        EnemyType enemyType = EnemyType::Unknown;
    };

    EnemyManager();
    ~EnemyManager();

    // Call once after OpenGL is ready
    void init();

    // Spawn enemies from all enemy_spawn entities in the map
    void spawnFromMap(const Map& map);

    bool spawn(EnemyType type,const glm::vec3& position);
    void setArenaEnabled(bool enabled);
    void setArenaDefinition(std::shared_ptr<const ArenaDefinition> definition);
    size_t count(EnemyType type) const;
    const std::vector<EnemyAction>& actions() const { return pendingActions; }

    void update(float deltaTime, const glm::vec3& playerPos, const Map* map);

    // Render all live enemies with the same point-light pipeline as the map
    void render(const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& ambientLight,
                const std::vector<MapRenderer::LightData>& lights);

    // Tests the swept bullet segment and reports the single affected enemy.
    BulletHitResult checkBulletHit(const glm::vec3& prevPos, const glm::vec3& pos, int damage);

    // Returns total contact damage per second from all touching enemies
    float getContactDps(const glm::vec3& playerPos) const;

    bool allEnemiesDead() const;
    bool hasSpawnedEnemies() const { return spawnedEnemyCount > 0; }
    size_t getLiveCount()  const;
    size_t getTotalCount() const { return spawnedEnemyCount; }

    // Returns XZ positions of all live enemies (used by minimap)
    std::vector<glm::vec3> getEnemyPositions() const;

    const std::vector<Enemy>& getEnemies() const { return enemies; }

    void clear();

private:
    std::vector<Enemy> enemies;
    std::unique_ptr<Shader> enemyShader;
    GLuint boxVAO = 0;
    GLuint boxVBO = 0;
    size_t spawnedEnemyCount = 0;
    uint32_t nextId=1;
    bool arenaEnabled=false;
    std::shared_ptr<const ArenaDefinition> arenaDefinition;
    std::vector<EnemyAction> pendingActions;

    void setupBoxMesh();
    void removeDeadEnemies();

    static constexpr float CONTACT_DPS = 20.0f; // HP/s per touching enemy
};

} // namespace silic2
