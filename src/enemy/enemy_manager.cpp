#include "enemy/enemy_manager.h"
#include "engine/shader.h"
#include "engine/collision.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>

namespace silic2 {

// Unit box: X[-0.5,0.5], Y[0,1], Z[-0.5,0.5] with per-face normals (36 vertices)
static const float BOX_VERTS[] = {
    // Front (Z+)
    -0.5f,0.0f, 0.5f,  0,0,1,   0.5f,0.0f, 0.5f,  0,0,1,   0.5f,1.0f, 0.5f,  0,0,1,
     0.5f,1.0f, 0.5f,  0,0,1,  -0.5f,1.0f, 0.5f,  0,0,1,  -0.5f,0.0f, 0.5f,  0,0,1,
    // Back (Z-)
     0.5f,0.0f,-0.5f,  0,0,-1, -0.5f,0.0f,-0.5f,  0,0,-1, -0.5f,1.0f,-0.5f,  0,0,-1,
    -0.5f,1.0f,-0.5f,  0,0,-1,  0.5f,1.0f,-0.5f,  0,0,-1,  0.5f,0.0f,-0.5f,  0,0,-1,
    // Left (X-)
    -0.5f,0.0f,-0.5f, -1,0,0,  -0.5f,0.0f, 0.5f, -1,0,0,  -0.5f,1.0f, 0.5f, -1,0,0,
    -0.5f,1.0f, 0.5f, -1,0,0,  -0.5f,1.0f,-0.5f, -1,0,0,  -0.5f,0.0f,-0.5f, -1,0,0,
    // Right (X+)
     0.5f,0.0f, 0.5f,  1,0,0,   0.5f,0.0f,-0.5f,  1,0,0,   0.5f,1.0f,-0.5f,  1,0,0,
     0.5f,1.0f,-0.5f,  1,0,0,   0.5f,1.0f, 0.5f,  1,0,0,   0.5f,0.0f, 0.5f,  1,0,0,
    // Top (Y+)
    -0.5f,1.0f, 0.5f,  0,1,0,   0.5f,1.0f, 0.5f,  0,1,0,   0.5f,1.0f,-0.5f,  0,1,0,
     0.5f,1.0f,-0.5f,  0,1,0,  -0.5f,1.0f,-0.5f,  0,1,0,  -0.5f,1.0f, 0.5f,  0,1,0,
    // Bottom (Y-)
    -0.5f,0.0f,-0.5f,  0,-1,0,  0.5f,0.0f,-0.5f,  0,-1,0,  0.5f,0.0f, 0.5f,  0,-1,0,
     0.5f,0.0f, 0.5f,  0,-1,0, -0.5f,0.0f, 0.5f,  0,-1,0, -0.5f,0.0f,-0.5f,  0,-1,0,
};

EnemyManager::EnemyManager() = default;

EnemyManager::~EnemyManager() {
    if (boxVAO) glDeleteVertexArrays(1, &boxVAO);
    if (boxVBO) glDeleteBuffers(1, &boxVBO);
}

void EnemyManager::init() {
    try {
        enemyShader = std::make_unique<Shader>(
            "res/shaders/enemy.vert",
            "res/shaders/enemy.frag"
        );
    } catch (const std::exception& e) {
        std::cerr << "EnemyManager: failed to load shader: " << e.what() << std::endl;
        throw;
    }
    setupBoxMesh();
}

void EnemyManager::setupBoxMesh() {
    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);

    glBindVertexArray(boxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(BOX_VERTS), BOX_VERTS, GL_STATIC_DRAW);

    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void EnemyManager::spawnFromMap(const Map& map) {
    enemies.clear();
    for (const auto& entity : map.getEntities()) {
        if (entity.type == EntityType::ENEMY_SPAWN) {
            enemies.emplace_back(entity.position);
            std::cout << "Spawned enemy at ("
                      << entity.position.x << ", "
                      << entity.position.y << ", "
                      << entity.position.z << ")\n";
        }
    }
    std::cout << "EnemyManager: " << enemies.size() << " enemy/enemies spawned.\n";
}

void EnemyManager::update(float deltaTime, const glm::vec3& playerPos, const Map* map) {
    for (auto& enemy : enemies) {
        enemy.update(deltaTime, playerPos, map);
    }
    removeDeadEnemies();
}

void EnemyManager::render(const glm::mat4& view, const glm::mat4& projection,
                           const glm::vec3& ambientLight,
                           const std::vector<MapRenderer::LightData>& lights) {
    if (!enemyShader || enemies.empty()) return;

    enemyShader->use();
    enemyShader->setMat4("view", view);
    enemyShader->setMat4("projection", projection);
    enemyShader->setVec3("ambientLight", ambientLight);
    enemyShader->setVec3("enemyColor", glm::vec3(1.0f, 0.25f, 0.05f));

    // Upload the same light array the map uses
    int numLights = std::min(static_cast<int>(lights.size()), 128);
    enemyShader->setInt("numLights", numLights);
    for (int i = 0; i < numLights; ++i) {
        std::string base = "lights[" + std::to_string(i) + "]";
        enemyShader->setVec3(base + ".position",  lights[i].position);
        enemyShader->setVec3(base + ".color",     lights[i].color);
        enemyShader->setFloat(base + ".intensity", lights[i].intensity);
        enemyShader->setFloat(base + ".range",     lights[i].range);
    }

    glBindVertexArray(boxVAO);

    for (const auto& enemy : enemies) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, enemy.getPosition());
        model = glm::scale(model, glm::vec3(Enemy::BOX_WIDTH, Enemy::BOX_HEIGHT, Enemy::BOX_WIDTH));

        enemyShader->setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindVertexArray(0);
}

// Segment-AABB intersection (slab method). Tests segment from 'start' to 'end' (t in [0,1]).
static bool segmentIntersectsAABB(const glm::vec3& start, const glm::vec3& end,
                                   const glm::vec3& bMin, const glm::vec3& bMax) {
    glm::vec3 d = end - start;
    float tMin = 0.0f, tMax = 1.0f;
    for (int i = 0; i < 3; ++i) {
        if (std::abs(d[i]) < 1e-8f) {
            if (start[i] < bMin[i] || start[i] > bMax[i]) return false;
        } else {
            float t1 = (bMin[i] - start[i]) / d[i];
            float t2 = (bMax[i] - start[i]) / d[i];
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            if (tMin > tMax) return false;
        }
    }
    return true;
}

bool EnemyManager::checkBulletHit(const glm::vec3& prevPos, const glm::vec3& pos, int damage) {
    for (auto& enemy : enemies) {
        if (enemy.isDead()) continue;

        AABB box = enemy.getAABB();
        if (segmentIntersectsAABB(prevPos, pos, box.min, box.max)) {
            enemy.takeDamage(damage);
            if (enemy.isDead()) {
                std::cout << "Enemy killed!\n";
            }
            return true;
        }
    }
    return false;
}

float EnemyManager::getContactDps(const glm::vec3& playerPos) const {
    float total = 0.0f;
    for (const auto& enemy : enemies) {
        if (!enemy.isDead() && enemy.isTouchingPlayer(playerPos)) {
            total += CONTACT_DPS;
        }
    }
    return total;
}

bool EnemyManager::allEnemiesDead() const {
    for (const auto& enemy : enemies) {
        if (!enemy.isDead()) return false;
    }
    return true;
}

size_t EnemyManager::getLiveCount() const {
    size_t count = 0;
    for (const auto& enemy : enemies) {
        if (!enemy.isDead()) ++count;
    }
    return count;
}

std::vector<glm::vec3> EnemyManager::getEnemyPositions() const {
    std::vector<glm::vec3> positions;
    for (const auto& enemy : enemies) {
        if (!enemy.isDead()) positions.push_back(enemy.getPosition());
    }
    return positions;
}

void EnemyManager::clear() {
    enemies.clear();
}

void EnemyManager::removeDeadEnemies() {
    enemies.erase(
        std::remove_if(enemies.begin(), enemies.end(),
            [](const Enemy& e) { return e.isDead(); }),
        enemies.end()
    );
}

} // namespace silic2
