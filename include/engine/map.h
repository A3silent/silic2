#pragma once
#include "engine/simple_json.h"
#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace silic2 {
enum class GameMode { Legacy, ArenaSurvival };
enum class EnvironmentPreset { None, StarryCrystal };
struct EnvironmentSettings {
    EnvironmentPreset preset = EnvironmentPreset::None;
    float transformationStart = 45, transformationDuration = 4;
    size_t particleCount = 256;
};
struct WorldSettings {
    float gravity = -9.8f;
    glm::vec3 ambientLight{.2f, .2f, .3f}, backgroundColor{.1f, .1f, .2f};
};
enum class SurfaceType { UNKNOWN, FLOOR, CEILING, WALL };
struct Brush {
    uint32_t id = 0;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<uint32_t> faces;
    std::string material = "default", texture;
    glm::vec3 color{.8f};
    SurfaceType surfaceType = SurfaceType::UNKNOWN;
    SimpleJson source; // Original extension fields retained during edits/saves.
};
enum class ObjectShape { Box, Plane };
struct GeometryObject {
    uint32_t id = 0;
    std::string name;
    ObjectShape shape = ObjectShape::Box;
    glm::vec3 position{0}, size{1}; // center and full dimensions; plane has size.y=0
    std::string material = "default", texture;
    glm::vec3 color{.8f};
    SurfaceType surfaceType = SurfaceType::UNKNOWN;
    SimpleJson source;
};
enum class EntityType {
    PLAYER_START,
    LIGHT,
    ENEMY_SPAWN,
    ITEM_PICKUP,
    TRIGGER,
    RIFT,
    SENTRY_SPAWN
};
struct Entity {
    EntityType type = EntityType::PLAYER_START;
    glm::vec3 position{0}, rotation{0}, scale{1}; // rotations in degrees; feet for spawns
    std::string name, stringProperty;
    glm::vec3 vec3Property{0};
    float floatProperty1 = 0, floatProperty2 = 0;
    int intProperty = 0, slot = -1;
    SimpleJson source;
};
enum class LightType { DIRECTIONAL, POINT, SPOT };
struct Light {
    LightType type = LightType::POINT;
    glm::vec3 position{0}, direction{0, -1, 0}, color{1};
    float intensity = 1, range = 10, innerConeAngle = 30, outerConeAngle = 45;
    int64_t anchorBrushId = -1;
    std::string anchorEntityName;
    glm::vec3 anchorOffset{0};
    SimpleJson source;
};
struct MapObstacle {
    uint32_t brushId = 0;
    glm::vec3 min{0}, max{0};
};
struct ArenaLayout {
    bool hasFloor = false, hasPlayerStart = false;
    glm::vec2 minXZ{0}, maxXZ{0};
    float floorY = 0;
    glm::vec3 playerStart{0};
    std::array<glm::vec3, 4> rifts{}, riftRotations{},
        riftScales{{glm::vec3(1), glm::vec3(1), glm::vec3(1), glm::vec3(1)}};
    std::array<bool, 4> riftActive{};
    std::array<glm::vec3, 3> sentryPositions{};
    std::array<bool, 3> sentryActive{};
    std::vector<MapObstacle> obstacles;
};

// Data-only shared module. All loads and editor mutations validate a temporary
// document first; false preserves the old scene and describes the error.
class Map {
  public:
    Map();
    ~Map() = default;
    bool loadFromFile(const std::string &filename);
    bool loadFromJson(const SimpleJson &document);
    bool saveToFile(const std::string &filename) const;
    SimpleJson toJson() const;
    void clear();
    bool validate() const;
    const std::string &getLastError() const {
        return lastError;
    }
    int getSchemaVersion() const {
        return schemaVersion;
    }
    GameMode getGameMode() const {
        return gameMode;
    }
    EnvironmentPreset getEnvironmentPreset() const {
        return environment.preset;
    }
    const EnvironmentSettings &getEnvironment() const {
        return environment;
    }
    const std::string &getContentVersion() const {
        return contentVersion;
    }
    const ArenaLayout &getArenaLayout() const {
        return arenaLayout;
    }
    const WorldSettings &getWorldSettings() const {
        return worldSettings;
    }
    const std::vector<Brush> &getBrushes() const {
        return brushes;
    }
    const std::vector<Brush> &getLegacyBrushes() const {
        return legacyBrushes;
    }
    const std::vector<GeometryObject> &getObjects() const {
        return objects;
    }
    const std::vector<Entity> &getEntities() const {
        return entities;
    }
    const std::vector<Light> &getLights() const {
        return lights;
    }
    const std::vector<Light> &getSourceLights() const {
        return sourceLights;
    }
    std::vector<const Brush *> getFloorBrushes() const;
    std::vector<const Brush *> getCeilingBrushes() const;
    std::vector<const Brush *> getWallBrushes() const;
    std::vector<const Brush *> getBrushesByType(SurfaceType type) const;
    const Entity *getPlayerStart() const;
    Entity *getPlayerStart(); // Compatibility; new editors use updateEntity.
    std::vector<Entity *> getEntitiesByType(EntityType type);
    bool setWorldSettings(const WorldSettings &settings);
    bool setGameMode(GameMode value);
    bool setEnvironment(const EnvironmentSettings &value);
    bool addBrush(const Brush &value);
    bool addObject(const GeometryObject &value);
    bool addEntity(const Entity &value);
    bool addLight(const Light &value);
    bool updateBrush(size_t index, const Brush &value); // authored legacy index
    bool updateObject(size_t index, const GeometryObject &value);
    bool updateEntity(size_t index, const Entity &value);
    bool updateLight(size_t index, const Light &value); // authored light index
    bool removeBrush(uint32_t id);
    bool removeObject(uint32_t id);
    bool removeEntity(size_t index);
    bool removeLight(size_t index);

  private:
    int schemaVersion = 1;
    GameMode gameMode = GameMode::Legacy;
    EnvironmentSettings environment;
    WorldSettings worldSettings;
    std::string contentVersion = "legacy", filename;
    mutable std::string lastError;
    SimpleJson sourceDocument;
    std::vector<Brush> legacyBrushes, brushes;
    std::vector<GeometryObject> objects;
    std::vector<Entity> entities;
    std::vector<Light> sourceLights, lights;
    ArenaLayout arenaLayout;
    int64_t floorBrushId = -1;
    void parseDocument(const SimpleJson &document);
    void rebuild();
    bool applyCandidate(const Map &candidate);
};
} // namespace silic2
