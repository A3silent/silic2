#include "weapon.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>

namespace silic2 {

Weapon::Weapon() : fireCooldown(0.0f), fireRate(0.2f), bulletVAO(0), bulletVBO(0), glowVAO(0), glowVBO(0), bulletLightingEnabled(false) {
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
    // 初始化子弹着色器
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
    // 创建长条形子弹网格（沿着Z轴的长方体）
    float bulletVertices[] = {
        // 前面
        -0.02f, -0.02f,  0.15f,  0.0f,  0.0f,  1.0f,
         0.02f, -0.02f,  0.15f,  0.0f,  0.0f,  1.0f,
         0.02f,  0.02f,  0.15f,  0.0f,  0.0f,  1.0f,
         0.02f,  0.02f,  0.15f,  0.0f,  0.0f,  1.0f,
        -0.02f,  0.02f,  0.15f,  0.0f,  0.0f,  1.0f,
        -0.02f, -0.02f,  0.15f,  0.0f,  0.0f,  1.0f,
        
        // 后面
        -0.02f, -0.02f, -0.15f,  0.0f,  0.0f, -1.0f,
         0.02f, -0.02f, -0.15f,  0.0f,  0.0f, -1.0f,
         0.02f,  0.02f, -0.15f,  0.0f,  0.0f, -1.0f,
         0.02f,  0.02f, -0.15f,  0.0f,  0.0f, -1.0f,
        -0.02f,  0.02f, -0.15f,  0.0f,  0.0f, -1.0f,
        -0.02f, -0.02f, -0.15f,  0.0f,  0.0f, -1.0f,
        
        // 左面
        -0.02f,  0.02f,  0.15f, -1.0f,  0.0f,  0.0f,
        -0.02f,  0.02f, -0.15f, -1.0f,  0.0f,  0.0f,
        -0.02f, -0.02f, -0.15f, -1.0f,  0.0f,  0.0f,
        -0.02f, -0.02f, -0.15f, -1.0f,  0.0f,  0.0f,
        -0.02f, -0.02f,  0.15f, -1.0f,  0.0f,  0.0f,
        -0.02f,  0.02f,  0.15f, -1.0f,  0.0f,  0.0f,
        
        // 右面
         0.02f,  0.02f,  0.15f,  1.0f,  0.0f,  0.0f,
         0.02f,  0.02f, -0.15f,  1.0f,  0.0f,  0.0f,
         0.02f, -0.02f, -0.15f,  1.0f,  0.0f,  0.0f,
         0.02f, -0.02f, -0.15f,  1.0f,  0.0f,  0.0f,
         0.02f, -0.02f,  0.15f,  1.0f,  0.0f,  0.0f,
         0.02f,  0.02f,  0.15f,  1.0f,  0.0f,  0.0f,
        
        // 上面
        -0.02f,  0.02f, -0.15f,  0.0f,  1.0f,  0.0f,
         0.02f,  0.02f, -0.15f,  0.0f,  1.0f,  0.0f,
         0.02f,  0.02f,  0.15f,  0.0f,  1.0f,  0.0f,
         0.02f,  0.02f,  0.15f,  0.0f,  1.0f,  0.0f,
        -0.02f,  0.02f,  0.15f,  0.0f,  1.0f,  0.0f,
        -0.02f,  0.02f, -0.15f,  0.0f,  1.0f,  0.0f,
        
        // 下面
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
    
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 法线属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void Weapon::setupGlowMesh() {
    // 创建一个更大的billboard四边形用于光晕效果
    float glowVertices[] = {
        // 位置              // 纹理坐标
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
    
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 纹理坐标属性
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void Weapon::update(float deltaTime, const Map* map) {
    // 更新射击冷却
    if (fireCooldown > 0.0f) {
        fireCooldown -= deltaTime;
    }
    
    // 更新所有子弹并检查碰撞
    for (auto it = bullets.begin(); it != bullets.end(); ) {
        it->update(deltaTime);
        
        // 检查碰撞
        if (map && checkBulletCollision(*it, map)) {
            // 在撞击点创建光效（仅当光照开启时）
            if (bulletLightingEnabled) {
                createImpactLight(it->position, it->color, it->intensity);
            }
            // 移除子弹
            it = bullets.erase(it);
        } else if (!it->isAlive()) {
            // 子弹超时
            it = bullets.erase(it);
        } else {
            ++it;
        }
    }
    
    // 更新撞击光效
    for (auto& light : impactLights) {
        light.update(deltaTime);
    }
    
    // 清理死亡光效
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

bool Weapon::checkBulletCollision(const Bullet& bullet, const Map* map) {
    if (!map) return false;
    
    const auto& brushes = map->getBrushes();
    
    // 对每个brush进行碰撞检测
    for (const auto& brush : brushes) {
        // 计算AABB
        float minX = 1e9f, minY = 1e9f, minZ = 1e9f;
        float maxX = -1e9f, maxY = -1e9f, maxZ = -1e9f;
        
        for (const auto& vertex : brush.vertices) {
            minX = std::min(minX, vertex.x);
            minY = std::min(minY, vertex.y);
            minZ = std::min(minZ, vertex.z);
            maxX = std::max(maxX, vertex.x);
            maxY = std::max(maxY, vertex.y);
            maxZ = std::max(maxZ, vertex.z);
        }
        
        // 子弹半径
        float bulletRadius = 0.05f;
        
        // 检查子弹是否碰撞到AABB的表面
        glm::vec3 pos = bullet.position;
        
        // 找到最近的点
        float closestX = glm::clamp(pos.x, minX, maxX);
        float closestY = glm::clamp(pos.y, minY, maxY);
        float closestZ = glm::clamp(pos.z, minZ, maxZ);
        
        // 计算到最近点的距离
        float distX = pos.x - closestX;
        float distY = pos.y - closestY;
        float distZ = pos.z - closestZ;
        float distSq = distX * distX + distY * distY + distZ * distZ;
        
        // 如果子弹中心到AABB最近点的距离小于子弹半径，则发生碰撞
        if (distSq < bulletRadius * bulletRadius) {
            // 额外检查：确保子弹不是从内部开始的（避免刚发射就碰撞）
            if (bullet.lifetime > 0.1f) {  // 子弹存活超过0.1秒才检测碰撞
                return true;
            }
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
        return; // 还在冷却中
    }
    
    // 获取相机位置和方向
    glm::vec3 cameraPos = camera.getPosition();
    glm::vec3 cameraFront = camera.getFront();
    glm::vec3 cameraRight = camera.getRight();
    glm::vec3 cameraUp = camera.getUp();
    
    // 计算子弹起始位置（屏幕右下角）
    glm::vec3 bulletStartPos = cameraPos;
    bulletStartPos += cameraFront * 0.3f;  // 向前偏移
    bulletStartPos += cameraRight * 0.2f;  // 向右偏移
    bulletStartPos -= cameraUp * 0.15f;    // 向下偏移
    
    // 子弹方向指向屏幕中心（相机前方）
    glm::vec3 bulletDirection = cameraFront;
    
    // 创建新子弹
    bullets.emplace_back(bulletStartPos, bulletDirection, 65.0f);
    
    // 重置冷却时间
    fireCooldown = fireRate;
}

void Weapon::render(const glm::mat4& view, const glm::mat4& projection) {
    if (bullets.empty()) {
        return;
    }
    
    // 首先渲染光晕（在子弹后面，使用混合）
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 加法混合
    glDepthMask(GL_FALSE); // 禁止写入深度缓冲
    
    glowShader->use();
    glowShader->setMat4("view", view);
    glowShader->setMat4("projection", projection);
    
    glBindVertexArray(glowVAO);
    
    // 渲染每个子弹的光晕
    for (const auto& bullet : bullets) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, bullet.position);
        
        // Billboard效果 - 始终面向相机
        glm::vec3 cameraRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
        glm::vec3 cameraUp = glm::vec3(view[0][1], view[1][1], view[2][1]);
        
        model[0] = glm::vec4(cameraRight * 0.8f, 0.0f); // 光晕大小
        model[1] = glm::vec4(cameraUp * 0.8f, 0.0f);
        model[2] = glm::vec4(0.0f, 0.0f, 0.8f, 0.0f);
        
        glowShader->setMat4("model", model);
        glowShader->setVec3("glowColor", bullet.color);
        glowShader->setFloat("intensity", bullet.intensity * 0.5f);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    glBindVertexArray(0);
    
    // 恢复深度写入和混合模式
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    
    // 然后渲染子弹本体
    bulletShader->use();
    bulletShader->setMat4("view", view);
    bulletShader->setMat4("projection", projection);
    
    glBindVertexArray(bulletVAO);
    
    // 渲染每个子弹
    for (const auto& bullet : bullets) {
        // 计算子弹的模型矩阵
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, bullet.position);
        
        // 根据子弹方向旋转
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
        
        // 设置着色器参数
        bulletShader->setMat4("model", model);
        bulletShader->setVec3("bulletColor", bullet.color);
        bulletShader->setFloat("intensity", bullet.intensity);
        
        // 绘制子弹
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    glBindVertexArray(0);
}

std::vector<std::pair<glm::vec3, glm::vec3>> Weapon::getActiveLights() const {
    std::vector<std::pair<glm::vec3, glm::vec3>> lights;
    
    // 如果子弹光照被禁用，只返回空列表（但子弹本身的光晕仍然显示）
    if (!bulletLightingEnabled) {
        return lights;
    }
    
    // 添加子弹光源
    for (const auto& bullet : bullets) {
        lights.emplace_back(bullet.getLightPosition(), bullet.getLightColor());
    }
    
    // 添加撞击光效
    for (const auto& impact : impactLights) {
        // 使用渐隐的强度
        glm::vec3 fadedColor = impact.color * impact.getFadedIntensity();
        lights.emplace_back(impact.position, fadedColor);
    }
    
    return lights;
}

} // namespace silic2