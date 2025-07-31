#include "collision.h"
#include <algorithm>
#include <limits>

namespace silic2 {

bool CollisionSystem::checkAABB(const AABB& a, const AABB& b) {
    return a.intersects(b);
}

CollisionResult CollisionSystem::sweepAABB(const AABB& movingBox, const glm::vec3& velocity, 
                                          const AABB& staticBox, float deltaTime) {
    CollisionResult result;
    
    // 扩展静态盒子的大小（Minkowski sum）
    AABB expandedBox;
    expandedBox.min = staticBox.min - movingBox.getSize() * 0.5f;
    expandedBox.max = staticBox.max + movingBox.getSize() * 0.5f;
    
    // 创建从移动盒子中心发出的射线
    glm::vec3 center = movingBox.getCenter();
    Ray ray(center, velocity);
    
    // 射线与扩展盒子的碰撞检测
    result = raycastAABB(ray, expandedBox, glm::length(velocity) * deltaTime);
    
    if (result.collided) {
        // 计算实际碰撞点（考虑盒子大小）
        glm::vec3 halfSize = movingBox.getSize() * 0.5f;
        
        // 根据法线方向调整碰撞点
        if (std::abs(result.normal.x) > 0.5f) {
            result.point.x += halfSize.x * -result.normal.x;
        }
        if (std::abs(result.normal.y) > 0.5f) {
            result.point.y += halfSize.y * -result.normal.y;
        }
        if (std::abs(result.normal.z) > 0.5f) {
            result.point.z += halfSize.z * -result.normal.z;
        }
    }
    
    return result;
}

CollisionResult CollisionSystem::raycastAABB(const Ray& ray, const AABB& box, float maxDistance) {
    CollisionResult result;
    
    glm::vec3 invDir = 1.0f / ray.direction;
    glm::vec3 t1 = (box.min - ray.origin) * invDir;
    glm::vec3 t2 = (box.max - ray.origin) * invDir;
    
    glm::vec3 tMin = glm::min(t1, t2);
    glm::vec3 tMax = glm::max(t1, t2);
    
    float tNear = std::max(std::max(tMin.x, tMin.y), tMin.z);
    float tFar = std::min(std::min(tMax.x, tMax.y), tMax.z);
    
    if (tNear > tFar || tFar < 0.0f || tNear > maxDistance) {
        return result; // 没有碰撞
    }
    
    result.collided = true;
    result.distance = tNear >= 0.0f ? tNear : tFar;
    result.point = ray.origin + ray.direction * result.distance;
    
    // 计算碰撞法线
    glm::vec3 center = box.getCenter();
    glm::vec3 localPoint = result.point - center;
    glm::vec3 halfSize = box.getSize() * 0.5f;
    
    float bias = 1.0001f;
    result.normal = glm::vec3(0.0f);
    
    if (std::abs(localPoint.x) > halfSize.x * 0.95f) {
        result.normal.x = localPoint.x > 0 ? 1.0f : -1.0f;
    } else if (std::abs(localPoint.y) > halfSize.y * 0.95f) {
        result.normal.y = localPoint.y > 0 ? 1.0f : -1.0f;
    } else if (std::abs(localPoint.z) > halfSize.z * 0.95f) {
        result.normal.z = localPoint.z > 0 ? 1.0f : -1.0f;
    }
    
    result.normal = glm::normalize(result.normal);
    
    return result;
}

glm::vec3 CollisionSystem::getAABBPenetration(const AABB& a, const AABB& b) {
    if (!a.intersects(b)) {
        return glm::vec3(0.0f);
    }
    
    glm::vec3 penetration(0.0f);
    
    // X轴
    float xOverlap = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
    if (xOverlap > 0) {
        if (a.getCenter().x < b.getCenter().x) {
            penetration.x = -xOverlap;
        } else {
            penetration.x = xOverlap;
        }
    }
    
    // Y轴
    float yOverlap = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
    if (yOverlap > 0) {
        if (a.getCenter().y < b.getCenter().y) {
            penetration.y = -yOverlap;
        } else {
            penetration.y = yOverlap;
        }
    }
    
    // Z轴
    float zOverlap = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);
    if (zOverlap > 0) {
        if (a.getCenter().z < b.getCenter().z) {
            penetration.z = -zOverlap;
        } else {
            penetration.z = zOverlap;
        }
    }
    
    return penetration;
}

glm::vec3 CollisionSystem::resolveAABBCollision(const AABB& movingBox, const AABB& staticBox) {
    glm::vec3 penetration = getAABBPenetration(movingBox, staticBox);
    
    if (penetration == glm::vec3(0.0f)) {
        return glm::vec3(0.0f);
    }
    
    // 找到最小穿透轴
    float minPenetration = std::numeric_limits<float>::max();
    glm::vec3 resolution(0.0f);
    
    if (std::abs(penetration.x) > 0.0f && std::abs(penetration.x) < minPenetration) {
        minPenetration = std::abs(penetration.x);
        resolution = glm::vec3(penetration.x, 0.0f, 0.0f);
    }
    
    if (std::abs(penetration.y) > 0.0f && std::abs(penetration.y) < minPenetration) {
        minPenetration = std::abs(penetration.y);
        resolution = glm::vec3(0.0f, penetration.y, 0.0f);
    }
    
    if (std::abs(penetration.z) > 0.0f && std::abs(penetration.z) < minPenetration) {
        minPenetration = std::abs(penetration.z);
        resolution = glm::vec3(0.0f, 0.0f, penetration.z);
    }
    
    return resolution;
}

} // namespace silic2