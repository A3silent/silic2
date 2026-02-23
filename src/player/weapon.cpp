#include "player/weapon.h"
#include "enemy/enemy_manager.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>

namespace silic2 {

Weapon::Weapon() : fireCooldown(0.0f), fireRate(0.06f), bulletVAO(0), bulletVBO(0), glowVAO(0), glowVBO(0), bulletLightingEnabled(false) {
}

Weapon::~Weapon() {
    if (bulletVAO) {
        glDeleteVertexArrays(1, &bulletVAO);
    }
    if (bulletVBO) {
        glDeleteBuffers(1, &bulletVBO);
    }
    if (glowVAO) {
        glDeleteVertexArrays(1, &glowVAO);
    }
    if (glowVBO) {
        glDeleteBuffers(1, &glowVBO);
    }
}

void Weapon::init() {
    // Initialize bullet shaders
    try {
        bulletShader = std::make_unique<Shader>("res/shaders/bullet.vert", "res/shaders/bullet.frag");
        glowShader = std::make_unique<Shader>("res/shaders/glow.vert", "res/shaders/glow.frag");
    } catch (const std::exception& e) {
        std::cerr << "Failed to load shaders: " << e.what() << std::endl;
        throw;
    }
    
    setupBulletMesh();
    setupGlowMesh();
}

void Weapon::setupBulletMesh() {
    // Create elongated bullet mesh (rectangular box along Z axis)
    float bulletVertices[] = {
        // Front face
        -0.02f, -0.02f,  0.15f,  0.0f,  0.0f,  1.0f,
         0.02f, -0.02f,  0.15f,  0.0f,  0.0f,  1.0f,
         0.02f,  0.02f,  0.15f,  0.0f,  0.0f,  1.0f,
         0.02f,  0.02f,  0.15f,  0.0f,  0.0f,  1.0f,
        -0.02f,  0.02f,  0.15f,  0.0f,  0.0f,  1.0f,
        -0.02f, -0.02f,  0.15f,  0.0f,  0.0f,  1.0f,
        
        // Back face
        -0.02f, -0.02f, -0.15f,  0.0f,  0.0f, -1.0f,
         0.02f, -0.02f, -0.15f,  0.0f,  0.0f, -1.0f,
         0.02f,  0.02f, -0.15f,  0.0f,  0.0f, -1.0f,
         0.02f,  0.02f, -0.15f,  0.0f,  0.0f, -1.0f,
        -0.02f,  0.02f, -0.15f,  0.0f,  0.0f, -1.0f,
        -0.02f, -0.02f, -0.15f,  0.0f,  0.0f, -1.0f,
        
        // Left face
        -0.02f,  0.02f,  0.15f, -1.0f,  0.0f,  0.0f,
        -0.02f,  0.02f, -0.15f, -1.0f,  0.0f,  0.0f,
        -0.02f, -0.02f, -0.15f, -1.0f,  0.0f,  0.0f,
        -0.02f, -0.02f, -0.15f, -1.0f,  0.0f,  0.0f,
        -0.02f, -0.02f,  0.15f, -1.0f,  0.0f,  0.0f,
        -0.02f,  0.02f,  0.15f, -1.0f,  0.0f,  0.0f,
        
        // Right face
         0.02f,  0.02f,  0.15f,  1.0f,  0.0f,  0.0f,
         0.02f,  0.02f, -0.15f,  1.0f,  0.0f,  0.0f,
         0.02f, -0.02f, -0.15f,  1.0f,  0.0f,  0.0f,
         0.02f, -0.02f, -0.15f,  1.0f,  0.0f,  0.0f,
         0.02f, -0.02f,  0.15f,  1.0f,  0.0f,  0.0f,
         0.02f,  0.02f,  0.15f,  1.0f,  0.0f,  0.0f,
        
        // Top face
        -0.02f,  0.02f, -0.15f,  0.0f,  1.0f,  0.0f,
         0.02f,  0.02f, -0.15f,  0.0f,  1.0f,  0.0f,
         0.02f,  0.02f,  0.15f,  0.0f,  1.0f,  0.0f,
         0.02f,  0.02f,  0.15f,  0.0f,  1.0f,  0.0f,
        -0.02f,  0.02f,  0.15f,  0.0f,  1.0f,  0.0f,
        -0.02f,  0.02f, -0.15f,  0.0f,  1.0f,  0.0f,
        
        // Bottom face
        -0.02f, -0.02f, -0.15f,  0.0f, -1.0f,  0.0f,
         0.02f, -0.02f, -0.15f,  0.0f, -1.0f,  0.0f,
         0.02f, -0.02f,  0.15f,  0.0f, -1.0f,  0.0f,
         0.02f, -0.02f,  0.15f,  0.0f, -1.0f,  0.0f,
        -0.02f, -0.02f,  0.15f,  0.0f, -1.0f,  0.0f,
        -0.02f, -0.02f, -0.15f,  0.0f, -1.0f,  0.0f
    };
    
    glGenVertexArrays(1, &bulletVAO);
    glGenBuffers(1, &bulletVBO);
    
    glBindVertexArray(bulletVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bulletVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bulletVertices), bulletVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void Weapon::setupGlowMesh() {
    // Create a larger billboard quad for glow effect
    float glowVertices[] = {
        // Position          // Texture coordinates
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f
    };
    
    glGenVertexArrays(1, &glowVAO);
    glGenBuffers(1, &glowVBO);
    
    glBindVertexArray(glowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, glowVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glowVertices), glowVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void Weapon::update(float deltaTime, const Map* map, EnemyManager* enemies) {
    // Update fire cooldown
    if (fireCooldown > 0.0f) {
        fireCooldown -= deltaTime;
    }
    
    // Update all bullets and check collisions
    for (auto it = bullets.begin(); it != bullets.end(); ) {
        it->update(deltaTime);
        
        // Check collision with map walls
        bool hitWall = map && checkBulletCollision(*it, map);
        if (hitWall && bulletLightingEnabled) {
            createImpactLight(it->position, it->color, it->intensity);
        }

        // Check collision with enemies using swept segment (prevents tunneling)
        bool hitEnemy = false;
        if (!hitWall && enemies && it->lifetime > 0.02f) {
            hitEnemy = enemies->checkBulletHit(it->prevPosition, it->position, 1);
        }

        if (hitWall || hitEnemy) {
            it = bullets.erase(it);
        } else if (!it->isAlive()) {
            // Bullet timeout
            it = bullets.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update impact lights
    for (auto& light : impactLights) {
        light.update(deltaTime);
    }
    
    // Clean up dead lights
    cleanupDeadLights();
}

void Weapon::cleanupDeadBullets() {
    bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(),
            [](const Bullet& b) { return !b.isAlive(); }),
        bullets.end()
    );
}

void Weapon::cleanupDeadLights() {
    impactLights.erase(
        std::remove_if(impactLights.begin(), impactLights.end(),
            [](const ImpactLight& l) { return !l.isAlive(); }),
        impactLights.end()
    );
}

// Segment-AABB slab test (t in [0,1])
static bool segmentHitsAABB(const glm::vec3& start, const glm::vec3& end,
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

bool Weapon::checkBulletCollision(const Bullet& bullet, const Map* map) {
    if (!map || bullet.lifetime <= 0.02f) return false;

    for (const auto& brush : map->getBrushes()) {
        if (brush.vertices.empty()) continue;
        glm::vec3 bMin = brush.vertices[0], bMax = brush.vertices[0];
        for (const auto& v : brush.vertices) {
            bMin = glm::min(bMin, v);
            bMax = glm::max(bMax, v);
        }
        if (segmentHitsAABB(bullet.prevPosition, bullet.position, bMin, bMax)) {
            return true;
        }
    }
    return false;
}

void Weapon::createImpactLight(const glm::vec3& position, const glm::vec3& color, float intensity) {
    impactLights.emplace_back(position, color, intensity);
    std::cout << "Created impact light at: " << position.x << ", " << position.y << ", " << position.z 
              << " | Total impact lights: " << impactLights.size() << std::endl;
}

void Weapon::fire(const Camera& camera) {
    if (fireCooldown > 0.0f) {
        return; // Still cooling down
    }
    
    // Get camera position and direction
    glm::vec3 cameraPos = camera.getPosition();
    glm::vec3 cameraFront = camera.getFront();
    glm::vec3 cameraRight = camera.getRight();
    glm::vec3 cameraUp = camera.getUp();
    
    // Calculate bullet start position (slight visual offset from gun)
    glm::vec3 bulletStartPos = cameraPos;
    bulletStartPos += cameraFront * 0.3f;
    bulletStartPos += cameraRight * 0.2f;
    bulletStartPos -= cameraUp * 0.15f;

    // Aim toward crosshair (point on camera center line 100 units out)
    // This converges the bullet to screen center regardless of spawn offset
    glm::vec3 aimPoint = cameraPos + cameraFront * 100.0f;
    glm::vec3 bulletDirection = glm::normalize(aimPoint - bulletStartPos);
    
    // Create new bullet
    bullets.emplace_back(bulletStartPos, bulletDirection, 65.0f);
    
    // Reset cooldown time
    fireCooldown = fireRate;
}

void Weapon::render(const glm::mat4& view, const glm::mat4& projection) {
    if (bullets.empty()) {
        return;
    }
    
    // First render glow (behind bullets, using blending)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending
    glDepthMask(GL_FALSE); // Disable depth buffer writing
    
    glowShader->use();
    glowShader->setMat4("view", view);
    glowShader->setMat4("projection", projection);
    
    glBindVertexArray(glowVAO);
    
    // Render glow for each bullet
    for (const auto& bullet : bullets) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, bullet.position);
        
        // Billboard effect - always face camera
        glm::vec3 cameraRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
        glm::vec3 cameraUp = glm::vec3(view[0][1], view[1][1], view[2][1]);
        
        model[0] = glm::vec4(cameraRight * 0.8f, 0.0f); // Glow size
        model[1] = glm::vec4(cameraUp * 0.8f, 0.0f);
        model[2] = glm::vec4(0.0f, 0.0f, 0.8f, 0.0f);
        
        glowShader->setMat4("model", model);
        glowShader->setVec3("glowColor", bullet.color);
        glowShader->setFloat("intensity", bullet.intensity * 0.5f);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    glBindVertexArray(0);
    
    // Restore depth writing and blend mode
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    
    // Then render bullet bodies
    bulletShader->use();
    bulletShader->setMat4("view", view);
    bulletShader->setMat4("projection", projection);
    
    glBindVertexArray(bulletVAO);
    
    // Render each bullet
    for (const auto& bullet : bullets) {
        // Calculate bullet model matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, bullet.position);
        
        // Rotate according to bullet direction
        glm::vec3 forward = bullet.direction;
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        if (glm::abs(glm::dot(forward, up)) > 0.99f) {
            up = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        glm::vec3 right = glm::normalize(glm::cross(up, forward));
        up = glm::cross(forward, right);
        
        glm::mat4 rotation = glm::mat4(1.0f);
        rotation[0] = glm::vec4(right, 0.0f);
        rotation[1] = glm::vec4(up, 0.0f);
        rotation[2] = glm::vec4(-forward, 0.0f);
        
        model = model * rotation;
        
        // Set shader parameters
        bulletShader->setMat4("model", model);
        bulletShader->setVec3("bulletColor", bullet.color);
        bulletShader->setFloat("intensity", bullet.intensity);
        
        // Draw bullet
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    glBindVertexArray(0);
}

std::vector<std::pair<glm::vec3, glm::vec3>> Weapon::getActiveLights() const {
    std::vector<std::pair<glm::vec3, glm::vec3>> lights;
    
    // If bullet lighting is disabled, just return empty list (but bullet glows still show)
    if (!bulletLightingEnabled) {
        return lights;
    }
    
    // Add bullet light sources
    for (const auto& bullet : bullets) {
        lights.emplace_back(bullet.getLightPosition(), bullet.getLightColor());
    }
    
    // Add impact lights
    for (const auto& impact : impactLights) {
        // Use fading intensity
        glm::vec3 fadedColor = impact.color * impact.getFadedIntensity();
        lights.emplace_back(impact.position, fadedColor);
    }
    
    return lights;
}

} // namespace silic2