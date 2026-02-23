#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace silic2 {

// Axis-Aligned Bounding Box (AABB)
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
    
    AABB() : min(0.0f), max(0.0f) {}
    AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}
    
    // Create from center point and half size
    static AABB fromCenterHalfSize(const glm::vec3& center, const glm::vec3& halfSize) {
        return AABB(center - halfSize, center + halfSize);
    }
    
    // Get center point
    glm::vec3 getCenter() const {
        return (min + max) * 0.5f;
    }
    
    // Get size
    glm::vec3 getSize() const {
        return max - min;
    }
    
    // Check if point is inside bounding box
    bool contains(const glm::vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }
    
    // Check if intersects with another AABB
    bool intersects(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }
};

// Ray
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    
    Ray(const glm::vec3& origin, const glm::vec3& direction) 
        : origin(origin), direction(glm::normalize(direction)) {}
};

// Collision result
struct CollisionResult {
    bool collided = false;
    glm::vec3 point;      // Collision point
    glm::vec3 normal;     // Collision normal
    float distance = 0.0f; // Collision distance
    
    CollisionResult() = default;
    CollisionResult(bool collided) : collided(collided) {}
};

// Collision detection system
class CollisionSystem {
public:
    // AABB vs AABB collision detection
    static bool checkAABB(const AABB& a, const AABB& b);
    
    // Moving AABB collision detection (sweep test)
    static CollisionResult sweepAABB(const AABB& movingBox, const glm::vec3& velocity, 
                                     const AABB& staticBox, float deltaTime);
    
    // Ray vs AABB collision detection
    static CollisionResult raycastAABB(const Ray& ray, const AABB& box, float maxDistance = 1000.0f);
    
    // Get AABB penetration depth and separation vector
    static glm::vec3 getAABBPenetration(const AABB& a, const AABB& b);
    
    // Resolve AABB collision (push out overlap)
    static glm::vec3 resolveAABBCollision(const AABB& movingBox, const AABB& staticBox);
};

} // namespace silic2