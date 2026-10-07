#include "enemy/enemy.h"
#include "engine/map.h"
#include "test_support.h"

#include <glm/glm.hpp>

using silic2::Brush;
using silic2::Enemy;
using silic2::Map;
using silic2::test::require;
using silic2::test::requireNear;

namespace {

Brush makeFloor(float y, float halfExtent) {
    Brush floor;
    floor.id = 1;
    floor.material = "floor";
    floor.vertices = {
        {-halfExtent, y, -halfExtent},
        {halfExtent, y, -halfExtent},
        {halfExtent, y, halfExtent},
        {-halfExtent, y, halfExtent}
    };
    floor.faces = {0, 1, 2, 2, 3, 0};
    return floor;
}

} // namespace

void runEnemyGroundTests() {
    Map map;
    require(map.addBrush(makeFloor(0.0f, 10.0f)), "valid floor fixture");

    constexpr float frameSteps[] = {0.008f, 0.016f, 0.033f, 0.05f};
    for (int spawn = 0; spawn < 1000; ++spawn) {
        Enemy enemy({0.0f, 1.0f, 0.0f});
        for (int frame = 0; frame < 200; ++frame) {
            const float dt = frameSteps[(spawn + frame) % 4];
            enemy.update(dt, {100.0f, 0.0f, 100.0f}, &map);
        }
        requireNear(enemy.getPosition().y, 0.05f, 0.001f,
                    "enemy crossed a zero-thickness floor while falling");
    }

    Enemy unsupported({20.0f, 1.0f, 20.0f});
    for (int i = 0; i < 20; ++i) {
        unsupported.update(0.05f, {100.0f, 0.0f, 100.0f}, &map);
    }
    require(unsupported.getPosition().y < 0.0f,
            "enemy landed on a floor outside its horizontal bounds");
}
