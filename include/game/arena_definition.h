#pragma once

#include <array>
#include <vector>
#include <glm/glm.hpp>

namespace silic2 {

class Map;

struct ArenaObstacle {
    glm::vec3 minimum;
    glm::vec3 maximum;
};

// Pure world-space definition. X/Z are ground-plane coordinates; Y is up.
struct ArenaDefinition {
    glm::vec2 minXZ{-18.0f}, maxXZ{18.0f};
    float floorY = 0.0f;
    glm::vec3 playerStart{0.0f, 0.0f, 13.0f};
    std::array<glm::vec3, 4> rifts{}; // N, E, S, W
    std::array<glm::vec3, 3> sentryPositions{};
    std::vector<ArenaObstacle> obstacles;
    std::vector<glm::vec3> waypoints;

    // Runtime definitions must come from validated map data.
    static ArenaDefinition fromMap(const Map& map);
    static ArenaDefinition starryCrystal(); // Explicit test fixture only.
    bool isWalkable(const glm::vec3& point, float radius = 0.65f) const;
    bool hasClearPath(const glm::vec3& from, const glm::vec3& to,
                      float radius = 0.65f) const;
    // First waypoint on a shortest visible route. Returns from if unreachable.
    glm::vec3 nextWaypoint(const glm::vec3& from, const glm::vec3& target,
                           float radius = 0.65f) const;
    bool hasConnectedWaypoints(float radius = 0.65f) const;

private:
    // Static edge costs are shared by successive queries on this definition.
    // Public geometry edits invalidate the cache through value comparison.
    void prepareNavigationEdges(float radius) const;
    mutable float cachedRadius = -1.0f;
    mutable std::vector<ArenaObstacle> cachedObstacles;
    mutable std::vector<glm::vec3> cachedWaypoints;
    mutable glm::vec2 cachedMinXZ{0}, cachedMaxXZ{0};
    mutable float cachedFloorY = 0.0f;
    mutable std::vector<glm::vec3> navigationPoints;
    std::vector<glm::vec3> buildNavigationPoints(float radius) const;
    mutable std::vector<std::vector<float>> cachedEdges;
};

} // namespace silic2
