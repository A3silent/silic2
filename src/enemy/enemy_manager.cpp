#include "enemy/enemy_manager.h"
#include "engine/shader.h"
#include "engine/collision.h"
#include "enemy/enemy_geometry.h"
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
    clear();
    setArenaDefinition(nullptr);
    for (const auto& entity : map.getEntities()) {
        if (entity.type == EntityType::ENEMY_SPAWN) {
            enemies.emplace_back(entity.position);
            enemies.back().setId(nextId++);
            std::cout << "Spawned enemy at ("
                      << entity.position.x << ", "
                      << entity.position.y << ", "
                      << entity.position.z << ")\n";
        }
    }
    spawnedEnemyCount = enemies.size();
    std::cout << "EnemyManager: " << enemies.size() << " enemy/enemies spawned.\n";
}

bool EnemyManager::spawn(EnemyType type,const glm::vec3& position) {
    if(type!=EnemyType::Chaser && type!=EnemyType::Sentry) return false;
    if(count(type)>=(type==EnemyType::Sentry ? 3u : 18u)) return false;
    enemies.emplace_back(position,type==EnemyType::Sentry ? 12 : 6,type);
    enemies.back().setId(nextId++);
    enemies.back().setArenaDefinition(arenaDefinition);
    enemies.back().setArenaEnabled(arenaEnabled);
    ++spawnedEnemyCount; return true;
}
void EnemyManager::setArenaEnabled(bool enabled) {
    arenaEnabled=enabled;
    for(auto& enemy:enemies) enemy.setArenaEnabled(enabled);
}
void EnemyManager::setArenaDefinition(std::shared_ptr<const ArenaDefinition> definition) {
    arenaDefinition=std::move(definition);
    arenaEnabled=bool(arenaDefinition);
    for(auto& enemy:enemies) enemy.setArenaDefinition(arenaDefinition);
}
size_t EnemyManager::count(EnemyType type) const {
    return std::count_if(enemies.begin(),enemies.end(),[&](const auto& e){return !e.isDead() && e.getType()==type;});
}
void EnemyManager::update(float deltaTime, const glm::vec3& playerPos, const Map* map) {
    pendingActions.clear();
    // Compute separation against the same snapshot for every actor.
    for(auto& enemy:enemies) {
        glm::vec3 separation(0);
        if(arenaEnabled && !enemy.isDead() && enemy.getType()==EnemyType::Chaser) {
            for(const auto& other:enemies) {
                if(other.getId()==enemy.getId() || other.isDead()) continue;
                auto offset=enemy.getPosition()-other.getPosition(); offset.y=0;
                float distance=glm::length(offset);
                if(distance>=1.0f) continue;
                if(distance<0.001f) {
                    // Paired opposite directions resolve exact spawn overlap.
                    const float angle=static_cast<float>(std::min(enemy.getId(),other.getId())%16)*0.392699f;
                    offset=glm::vec3(std::cos(angle),0,std::sin(angle))*(enemy.getId()<other.getId() ? 1.0f : -1.0f);
                } else offset/=distance;
                separation+=offset*(1.0f-distance)*1.2f;
            }
            if(glm::length(separation)>1.2f) separation=glm::normalize(separation)*1.2f;
        }
        enemy.setSeparation(separation);
    }
    for (auto& enemy : enemies) {
        enemy.update(deltaTime, playerPos, map);
        pendingActions.insert(pendingActions.end(),enemy.actions().begin(),enemy.actions().end());
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
        model = glm::scale(model, enemy.getAABB().getSize());

        enemyShader->setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindVertexArray(0);
}

EnemyManager::BulletHitResult EnemyManager::checkBulletHit(
    const glm::vec3& prevPos, const glm::vec3& pos, int damage) {
    Enemy* nearest=nullptr; float fraction=2;
    for(auto& enemy:enemies) {
        if(enemy.isDead()) continue;
        float t;
        if(enemyGeometry::segment(prevPos,pos,enemy.getAABB(),t) && t<fraction) { nearest=&enemy; fraction=t; }
    }
    if(!nearest) return {};
    nearest->takeDamage(damage);
    return {true,nearest->isDead(),nearest->getType()};
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
    spawnedEnemyCount = 0;
    nextId=1;
    pendingActions.clear();
}

void EnemyManager::removeDeadEnemies() {
    enemies.erase(
        std::remove_if(enemies.begin(), enemies.end(),
            [](const Enemy& e) { return e.isDead(); }),
        enemies.end()
    );
}

} // namespace silic2
