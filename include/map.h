#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include "simple_json.h"

namespace silic2 {

struct WorldSettings {
    float gravity = -9.8f;
    glm::vec3 ambientLight = glm::vec3(0.2f, 0.2f, 0.3f);
    glm::vec3 backgroundColor = glm::vec3(0.1f, 0.1f, 0.2f);
};

enum class SurfaceType {
    UNKNOWN,
    FLOOR,
    CEILING,
    WALL
};

struct Brush {
    uint32_t id;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;  // 纹理坐标
    std::vector<uint32_t> faces;  // indices into vertices array (triangles)
    std::string material;
    std::string texture;  // 纹理文件路径
    glm::vec3 color = glm::vec3(0.8f, 0.8f, 0.8f);
    SurfaceType surfaceType = SurfaceType::UNKNOWN;  // Automatically determined from geometry
};

enum class EntityType {
    PLAYER_START,
    LIGHT,
    ENEMY_SPAWN,
    ITEM_PICKUP,
    TRIGGER
};

struct Entity {
    EntityType type;
    glm::vec3 position;
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    std::string name;
    
    // Simplified properties - avoid union with complex types
    std::string stringProperty;  // For enemyType, itemType, etc.
    glm::vec3 vec3Property;      // For light color, etc.
    float floatProperty1 = 0.0f; // For intensity, health, etc.
    float floatProperty2 = 0.0f; // For range, etc.
    int intProperty = 0;         // For quantity, etc.
};

enum class LightType {
    DIRECTIONAL,
    POINT,
    SPOT
};

struct Light {
    LightType type;
    glm::vec3 position;
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    float intensity = 1.0f;
    float range = 10.0f;
    float innerConeAngle = 30.0f;  // For spot lights
    float outerConeAngle = 45.0f;  // For spot lights
};

class Map {
public:
    Map();
    ~Map();

    // Load/Save
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename) const;
    void clear();

    // Getters
    const WorldSettings& getWorldSettings() const { return worldSettings; }
    const std::vector<Brush>& getBrushes() const { return brushes; }
    const std::vector<Entity>& getEntities() const { return entities; }
    const std::vector<Light>& getLights() const { return lights; }
    
    // Geometry separation getters
    std::vector<const Brush*> getFloorBrushes() const;
    std::vector<const Brush*> getCeilingBrushes() const;
    std::vector<const Brush*> getWallBrushes() const;
    std::vector<const Brush*> getBrushesByType(SurfaceType type) const;
    
    // Entity queries
    Entity* getPlayerStart();
    std::vector<Entity*> getEntitiesByType(EntityType type);
    
    // Modifiers (for editor)
    void setWorldSettings(const WorldSettings& settings) { worldSettings = settings; }
    void addBrush(const Brush& brush) { brushes.push_back(brush); }
    void addEntity(const Entity& entity) { entities.push_back(entity); }
    void addLight(const Light& light) { lights.push_back(light); }
    
    void removeBrush(uint32_t id);
    void removeEntity(size_t index);
    void removeLight(size_t index);
    
    // Validation
    bool validate() const;

private:
    WorldSettings worldSettings;
    std::vector<Brush> brushes;
    std::vector<Entity> entities;
    std::vector<Light> lights;
    
    std::string filename;
    bool loaded = false;
    
    // Helper functions for JSON parsing
    bool parseWorldSettings(const SimpleJson& json);
    bool parseBrushes(const SimpleJson& json);
    bool parseEntities(const SimpleJson& json);
    bool parseLights(const SimpleJson& json);
    
    // Geometry analysis helpers
    void analyzeSurfaceTypes();
    SurfaceType determineSurfaceType(const Brush& brush);
    glm::vec3 calculateFaceNormal(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
    
    // Helper functions for JSON writing
    SimpleJson worldSettingsToJson() const;
    SimpleJson brushesToJson() const;
    SimpleJson entitiesToJson() const;
    SimpleJson lightsToJson() const;
};

} // namespace silic2