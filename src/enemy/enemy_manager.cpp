#include "enemy/enemy_manager.h"
#include "enemy/crystal_combat.h"
#include "game/encounter_director.h"
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
    size_t limit;
    int hp;
    switch(type) {
        case EnemyType::Chaser:limit=18;hp=6;break;
        case EnemyType::Sentry:limit=3;hp=12;break;
        case EnemyType::ForkHound:limit=2;hp=crystalCombat::HoundHp;break;
        case EnemyType::VeilRay:limit=2;hp=crystalCombat::RayHp;break;
        case EnemyType::HangingCenser:limit=1;hp=crystalCombat::CenserHp;break;
        default:return false;
    }
    if(arenaDefinition && arenaDefinition->encounterProfile==EncounterProfile::CrystalSurvival)
        limit=EncounterDirector::crystalMaxAlive(type);
    if(count(type)>=limit) return false;
    enemies.emplace_back(position,hp,type);
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
void EnemyManager::update(float deltaTime, const glm::vec3& playerPos, const Map* map,const AABB* playerBounds,size_t inFlightVolleys) {
    pendingActions.clear();
    if(deltaTime<=0) return;
    if(havePlayer) {
        auto v=(playerPos-lastPlayer)/deltaTime;v.y=0;
        if(glm::length(v)>18.f) v=glm::normalize(v)*18.f;
        filteredVelocity=glm::mix(filteredVelocity,v,1-std::exp(-deltaTime*12.f));
    }
    havePlayer=true;lastPlayer=playerPos;
    heavyCooldown=std::max(0.f,heavyCooldown-deltaTime);
    const AABB bounds=playerBounds ? *playerBounds : AABB(playerPos+glm::vec3(-.3f,0,-.3f),playerPos+glm::vec3(.3f,1.8f,.3f));
    auto heavy=[](const Enemy& e) {
        const auto phase=e.getAttackPhase();
        return !e.isDead() && (phase==AttackPhase::Windup || phase==AttackPhase::Attack);
    };
    int activeHeavy=static_cast<int>(inFlightVolleys)+static_cast<int>(std::count_if(enemies.begin(),enemies.end(),heavy));
    // Compute separation against the same snapshot for every actor.
    for(auto& enemy:enemies) {
        glm::vec3 separation(0);
        if(arenaEnabled && !enemy.isDead() && enemy.getType()==EnemyType::Chaser) {
            const float spacing=pressureStage ? 1.35f : 1.f;
            const float push=pressureStage ? 3.8f : 1.2f;
            for(const auto& other:enemies) {
                if(other.getId()==enemy.getId() || other.isDead()) continue;
                auto offset=enemy.getPosition()-other.getPosition(); offset.y=0;
                if(std::abs(offset.x)>=spacing || std::abs(offset.z)>=spacing ||
                   std::abs(enemy.getPosition().y-other.getPosition().y)>2) continue;
                float distance=glm::length(offset);
                if(distance>=spacing) continue;
                if(distance<0.001f) {
                    // Paired opposite directions resolve exact spawn overlap.
                    const float angle=static_cast<float>(std::min(enemy.getId(),other.getId())%16)*0.392699f;
                    offset=glm::vec3(std::cos(angle),0,std::sin(angle))*(enemy.getId()<other.getId() ? 1.0f : -1.0f);
                } else offset/=distance;
                separation+=offset*(1-distance/spacing)*push;
            }
            if(glm::length(separation)>push) separation=glm::normalize(separation)*push;
        }
        enemy.setSeparation(separation);
    }
    constexpr int simultaneous[]={2,2,3,4,7};
    constexpr float spacing[]={crystalCombat::HeavySpacing,crystalCombat::HeavySpacing,1.05f,.65f,.36f};
    // Rotate attack permission past the previous attacker so the oldest pack
    // cannot starve the ray or censer when many creatures are ready together.
    const auto first=pressureStage>=2 ? std::upper_bound(enemies.begin(),enemies.end(),lastHeavyId,
        [](uint32_t id,const Enemy& enemy){return id<enemy.getId();}) : enemies.begin();
    size_t start=static_cast<size_t>(first-enemies.begin());
    for (size_t i=0;i<enemies.size();++i) {
        auto& enemy=enemies[(start+i)%enemies.size()];
        const bool before=heavy(enemy);
        enemy.setPressureStage(pressureStage);
        enemy.setAttackContext(heavyCooldown<=0 && activeHeavy<simultaneous[pressureStage],filteredVelocity,bounds);
        enemy.update(deltaTime, playerPos, map);
        activeHeavy+=int(heavy(enemy))-int(before);
        for(const auto& action:enemy.actions()) {
            if(action.kind==EnemyAction::Kind::Windup) {
                heavyCooldown=spacing[pressureStage];
                lastHeavyId=enemy.getId();
            }
            if(action.kind==EnemyAction::Kind::Release && action.type==EnemyType::HangingCenser) ++activeHeavy;
        }
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
    const glm::vec3& prevPos, const glm::vec3& pos, int damage,float maxFraction) {
    Enemy* nearest=nullptr; float fraction=maxFraction; bool gland=false;
    for(auto& enemy:enemies) {
        if(enemy.isDead()) continue;
        float t;
        bool weak;
        if(enemy.intersectShot(prevPos,pos,t,weak) && t<fraction) {
            nearest=&enemy;fraction=t;gland=weak;
        }
    }
    if(!nearest) return {};
    nearest->receiveShot(damage,gland);
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
    nextId=1;heavyCooldown=0;lastHeavyId=0;pressureStage=0;havePlayer=false;lastPlayer=filteredVelocity=glm::vec3(0);
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
