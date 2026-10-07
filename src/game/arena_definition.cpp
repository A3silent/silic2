#include "game/arena_definition.h"
#include "engine/map.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace silic2 {
namespace {
float groundDistance(const glm::vec3& a, const glm::vec3& b) {
    return glm::length(glm::vec2(a.x - b.x, a.z - b.z));
}

bool segmentIntersects(const glm::vec3& a, const glm::vec3& b,
                       const ArenaObstacle& obstacle, float radius) {
    float enter = 0.0f;
    float leave = 1.0f;
    for (int axis : {0, 2}) {
        const float lower = obstacle.minimum[axis] - radius;
        const float upper = obstacle.maximum[axis] + radius;
        const float delta = b[axis] - a[axis];
        if (std::abs(delta) < 1e-7f) {
            if (a[axis] < lower || a[axis] > upper) return false;
        } else {
            float first = (lower - a[axis]) / delta;
            float last = (upper - a[axis]) / delta;
            if (first > last) std::swap(first, last);
            enter = std::max(enter, first);
            leave = std::min(leave, last);
            if (enter > leave) return false;
        }
    }
    return true;
}

// Navigation adds clearance beyond the physical collision box. A legal actor
// can therefore enter this margin through player movement or enemy separation.
// Project locally out of the margin, retaining the side of the solid geometry.
bool projectNavigationMargin(const ArenaDefinition& arena, const glm::vec3& point,
                             float radius, glm::vec3& projected) {
    if (arena.isWalkable(point, radius)) { projected = point; return true; }
    if (!arena.isWalkable(point, 0.0f)) return false;
    // More than Enemy's .01 movement deadzone, even at the clearance boundary.
    constexpr float recoveryMargin = .02f;
    const glm::vec2 lower = arena.minXZ + radius + recoveryMargin;
    const glm::vec2 upper = arena.maxXZ - radius - recoveryMargin;
    if (lower.x > upper.x || lower.y > upper.y) return false;
    glm::vec3 clamped = point;
    clamped.x = std::clamp(clamped.x, lower.x, upper.x);
    clamped.z = std::clamp(clamped.z, lower.y, upper.y);
    float nearest = std::numeric_limits<float>::infinity();
    auto consider = [&](const glm::vec3& candidate) {
        const float distance = groundDistance(point, candidate);
        if (distance >= nearest || !arena.isWalkable(candidate, radius) ||
            !arena.hasClearPath(point, candidate, 0.0f)) return;
        nearest = distance;
        projected = candidate;
    };
    consider(clamped);
    for (const auto& obstacle : arena.obstacles) {
        if (clamped.x < obstacle.minimum.x - radius || clamped.x > obstacle.maximum.x + radius ||
            clamped.z < obstacle.minimum.z - radius || clamped.z > obstacle.maximum.z + radius) continue;
        for (int axis : {0, 2}) {
            auto candidate = clamped;
            candidate[axis] = obstacle.minimum[axis] - radius - recoveryMargin;
            consider(candidate);
            candidate[axis] = obstacle.maximum[axis] + radius + recoveryMargin;
            consider(candidate);
        }
    }
    return std::isfinite(nearest);
}
} // namespace

ArenaDefinition ArenaDefinition::fromMap(const Map& map) {
    const auto& layout = map.getArenaLayout();
    if (map.getGameMode() != GameMode::ArenaSurvival || !layout.hasFloor)
        throw std::invalid_argument("ArenaDefinition requires a validated arena_survival map");
    ArenaDefinition arena;
    arena.minXZ = layout.minXZ;
    arena.maxXZ = layout.maxXZ;
    arena.floorY = layout.floorY;
    arena.playerStart = layout.playerStart;
    arena.rifts = layout.rifts;
    arena.sentryPositions = layout.sentryPositions;
    for (const auto& obstacle : layout.obstacles)
        arena.obstacles.push_back({obstacle.min, obstacle.max});
    arena.waypoints = arena.buildNavigationPoints(.65f);
    return arena;
}

ArenaDefinition ArenaDefinition::starryCrystal() {
    ArenaDefinition arena;
    arena.rifts = {{{0, 0, -15}, {15, 0, 0}, {0, 0, 15}, {-15, 0, 0}}};
    arena.sentryPositions = {{{-11, 0, -10}, {11, 0, -10}, {10, 0, 10}}};
    arena.obstacles = {{{-2, 0, -2}, {2, 4.2f, 2}},
                       {{-10, 0, -2}, {-8, 1.4f, 2}},
                       {{8, 0, -2}, {10, 1.4f, 2}}};
    arena.waypoints = arena.buildNavigationPoints(.65f);
    return arena;
}

bool ArenaDefinition::isWalkable(const glm::vec3& point, float radius) const {
    if (radius < 0.0f || !std::isfinite(point.x) || !std::isfinite(point.z) ||
        !std::isfinite(radius) ||
        point.x < minXZ.x + radius || point.x > maxXZ.x - radius ||
        point.z < minXZ.y + radius || point.z > maxXZ.y - radius) return false;
    for (const auto& obstacle : obstacles) {
        if (point.x >= obstacle.minimum.x - radius &&
            point.x <= obstacle.maximum.x + radius &&
            point.z >= obstacle.minimum.z - radius &&
            point.z <= obstacle.maximum.z + radius) return false;
    }
    return true;
}

bool ArenaDefinition::hasClearPath(const glm::vec3& from, const glm::vec3& to,
                                  float radius) const {
    if (!isWalkable(from, radius) || !isWalkable(to, radius)) return false;
    for (const auto& obstacle : obstacles) {
        if (segmentIntersects(from, to, obstacle, radius)) return false;
    }
    return true;
}

glm::vec3 ArenaDefinition::nextWaypoint(const glm::vec3& from,
                                       const glm::vec3& target, float radius) const {
    if (!std::isfinite(radius) || radius < 0.0f) return from;
    if (hasClearPath(from, target, radius)) return target;
    glm::vec3 origin;
    glm::vec3 destination;
    if (!projectNavigationMargin(*this, from, radius, origin)) return from;
    std::vector<glm::vec3> destinations;
    if (projectNavigationMargin(*this, target, radius, destination)) {
        destinations.push_back(destination);
    } else {
        // Ground navigation remains solid beneath a player on a low cover.
        // Only the destination may be lifted above that solid: pursue one of
        // its ground edges, selected by reachable route cost from this actor.
        // An origin inside a solid still fails the independent check above.
        for (const auto& obstacle : obstacles) {
            if (!std::isfinite(target.y) || target.y + 1e-4f < obstacle.maximum.y ||
                target.x < obstacle.minimum.x || target.x > obstacle.maximum.x ||
                target.z < obstacle.minimum.z || target.z > obstacle.maximum.z) continue;
            for (int axis : {0, 2}) {
                for (int side : {-1, 1}) {
                    auto candidate = target;
                    candidate.y = from.y;
                    candidate[axis] = side < 0 ? obstacle.minimum[axis] - radius - .02f :
                                                obstacle.maximum[axis] + radius + .02f;
                    if (isWalkable(candidate, radius)) destinations.push_back(candidate);
                }
            }
        }
    }
    if (destinations.empty()) return from;
    // Recover outward first. Routing straight from a margin origin could cut
    // across the obstruction before reaching an otherwise valid graph node.
    if (origin != from) return origin;
    if (destinations.size() == 1 && hasClearPath(from, destinations[0], radius)) return destinations[0];
    prepareNavigationEdges(radius);
    const size_t firstTarget = navigationPoints.size();
    const size_t count = firstTarget + destinations.size();
    const float infinity = std::numeric_limits<float>::infinity();
    std::vector<float> distance(count, infinity);
    std::vector<size_t> first(count, count);
    std::vector<bool> visited(count, false);
    std::vector<float> targetEdges(firstTarget * destinations.size(), infinity);
    auto point = [&](size_t index) { return index >= firstTarget ? destinations[index-firstTarget] : navigationPoints[index]; };
    for (size_t i = 0; i < count; ++i) {
        if (hasClearPath(from, point(i), radius)) {
            distance[i] = groundDistance(from, point(i));
            first[i] = i;
        }
        if (i < firstTarget) {
            for (size_t goal = 0; goal < destinations.size(); ++goal) {
                if (hasClearPath(point(i), destinations[goal], radius))
                    targetEdges[i * destinations.size() + goal] = groundDistance(point(i), destinations[goal]);
            }
        }
    }
    for (size_t iteration = 0; iteration < count; ++iteration) {
        size_t selected = count;
        for (size_t i = 0; i < count; ++i) {
            if (!visited[i] && std::isfinite(distance[i]) &&
                (selected == count || distance[i] < distance[selected])) selected = i;
        }
        if (selected == count) break;
        // The first settled destination is the nearest reachable ground edge.
        if (selected >= firstTarget) return point(first[selected]);
        visited[selected] = true;
        for (size_t i = 0; i < count; ++i) {
            if (visited[i]) continue;
            const float edge = i >= firstTarget ? targetEdges[selected * destinations.size() + i-firstTarget] : cachedEdges[selected][i];
            if (!std::isfinite(edge)) continue;
            const float candidate = distance[selected] + edge;
            if (candidate < distance[i] - 1e-5f) {
                distance[i] = candidate;
                first[i] = first[selected];
            }
        }
    }
    return from;
}

std::vector<glm::vec3> ArenaDefinition::buildNavigationPoints(float radius) const {
    // Visibility vertices derive from each inflated obstacle, not from a fixed
    // grid. Thus translating or resizing cover changes the route immediately.
    // Clearance beyond the corner exceeds Enemy's .01 arrival deadzone.
    // This allows pursuit to continue around a corner before its next frame.
    constexpr float epsilon = .02f;
    std::vector<glm::vec3> points;
    auto add = [&](float x, float z) {
        const glm::vec3 candidate{x, floorY, z};
        if (isWalkable(candidate, radius)) points.push_back(candidate);
    };
    for (float x : {minXZ.x + radius + epsilon, maxXZ.x - radius - epsilon})
        for (float z : {minXZ.y + radius + epsilon, maxXZ.y - radius - epsilon}) add(x,z);
    for (const auto& obstacle : obstacles)
        for (float x : {obstacle.minimum.x-radius-epsilon, obstacle.maximum.x+radius+epsilon})
            for (float z : {obstacle.minimum.z-radius-epsilon, obstacle.maximum.z+radius+epsilon}) add(x,z);
    add(playerStart.x, playerStart.z);
    for (const auto& point : rifts) add(point.x, point.z);
    for (const auto& point : sentryPositions) add(point.x, point.z);
    // Existing explicitly supplied waypoints are optional extra vertices. Never
    // retain an invalid point after a geometry edit or at a larger query radius.
    for (const auto& point : waypoints) add(point.x, point.z);
    std::sort(points.begin(), points.end(), [](const auto& a, const auto& b) {
        return a.z < b.z || (a.z == b.z && a.x < b.x);
    });
    points.erase(std::unique(points.begin(), points.end()), points.end());
    return points;
}

void ArenaDefinition::prepareNavigationEdges(float radius) const {
    bool same = cachedRadius == radius && cachedMinXZ == minXZ && cachedMaxXZ == maxXZ &&
        cachedFloorY == floorY && cachedWaypoints.size() == waypoints.size() && cachedObstacles.size() == obstacles.size();
    for (size_t i = 0; same && i < waypoints.size(); ++i) same = cachedWaypoints[i] == waypoints[i];
    for (size_t i = 0; same && i < obstacles.size(); ++i)
        same = cachedObstacles[i].minimum == obstacles[i].minimum && cachedObstacles[i].maximum == obstacles[i].maximum;
    if (same) return;
    cachedRadius = radius;
    cachedMinXZ = minXZ;
    cachedMaxXZ = maxXZ;
    cachedFloorY = floorY;
    cachedWaypoints = waypoints;
    cachedObstacles = obstacles;
    navigationPoints = buildNavigationPoints(radius);
    cachedEdges.assign(navigationPoints.size(), std::vector<float>(navigationPoints.size(), std::numeric_limits<float>::infinity()));
    for (size_t i = 0; i < navigationPoints.size(); ++i) {
        for (size_t j = i + 1; j < navigationPoints.size(); ++j) {
            if (hasClearPath(navigationPoints[i], navigationPoints[j], radius))
                cachedEdges[i][j] = cachedEdges[j][i] = groundDistance(navigationPoints[i], navigationPoints[j]);
        }
    }
}

bool ArenaDefinition::hasConnectedWaypoints(float radius) const {
    prepareNavigationEdges(radius);
    if (navigationPoints.empty()) return false;
    std::vector<bool> reached(navigationPoints.size(), false);
    std::vector<size_t> pending{0};
    reached[0] = true;
    for (size_t index = 0; index < pending.size(); ++index) {
        const size_t i = pending[index];
        for (size_t j = 0; j < navigationPoints.size(); ++j) {
            if (!reached[j] && std::isfinite(cachedEdges[i][j])) {
                reached[j] = true;
                pending.push_back(j);
            }
        }
    }
    return std::all_of(reached.begin(), reached.end(), [](bool value) { return value; });
}

} // namespace silic2
