#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace silic2 {

// 轴对齐包围盒（AABB）
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
    
    AABB() : min(0.0f), max(0.0f) {}
    AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}
    
    // 从中心点和半尺寸创建
    static AABB fromCenterHalfSize(const glm::vec3& center, const glm::vec3& halfSize) {
        return AABB(center - halfSize, center + halfSize);
    }
    
    // 获取中心点
    glm::vec3 getCenter() const {
        return (min + max) * 0.5f;
    }
    
    // 获取尺寸
    glm::vec3 getSize() const {
        return max - min;
    }
    
    // 检查点是否在包围盒内
    bool contains(const glm::vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }
    
    // 检查与另一个AABB是否相交
    bool intersects(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }
};

// 射线
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    
    Ray(const glm::vec3& origin, const glm::vec3& direction) 
        : origin(origin), direction(glm::normalize(direction)) {}
};

// 碰撞结果
struct CollisionResult {
    bool collided = false;
    glm::vec3 point;      // 碰撞点
    glm::vec3 normal;     // 碰撞法线
    float distance = 0.0f; // 碰撞距离
    
    CollisionResult() = default;
    CollisionResult(bool collided) : collided(collided) {}
};

// 碰撞检测系统
class CollisionSystem {
public:
    // AABB vs AABB 碰撞检测
    static bool checkAABB(const AABB& a, const AABB& b);
    
    // 移动AABB碰撞检测（扫掠测试）
    static CollisionResult sweepAABB(const AABB& movingBox, const glm::vec3& velocity, 
                                     const AABB& staticBox, float deltaTime);
    
    // 射线与AABB碰撞检测
    static CollisionResult raycastAABB(const Ray& ray, const AABB& box, float maxDistance = 1000.0f);
    
    // 获取AABB穿透深度和分离向量
    static glm::vec3 getAABBPenetration(const AABB& a, const AABB& b);
    
    // 解决AABB碰撞（推出重叠）
    static glm::vec3 resolveAABBCollision(const AABB& movingBox, const AABB& staticBox);
};

} // namespace silic2