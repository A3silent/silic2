#include "engine/map.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace silic2 {

Map::Map() {
    clear();
}

Map::~Map() {
    clear();
}

bool Map::loadFromFile(const std::string& filename) {
    std::cout << "Loading map: " << filename << std::endl;
    
    std::string json_content;
    
    // Read actual file content
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open map file: " << filename << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    json_content = buffer.str();
    file.close();
    
    std::cout << "Successfully read " << json_content.length() << " bytes from " << filename << std::endl;
    
    try {
        SimpleJson json = SimpleJson::parse(json_content);
        
        if (!json.isObject()) {
            std::cerr << "Invalid JSON format in map file" << std::endl;
            return false;
        }

        // Parse each section
        if (json.hasKey("worldSettings")) {
            if (!parseWorldSettings(json["worldSettings"])) {
                std::cerr << "Failed to parse world settings" << std::endl;
                return false;
            }
        }

        if (json.hasKey("geometry")) {
            if (!parseBrushes(json["geometry"])) {
                std::cerr << "Failed to parse geometry" << std::endl;
                return false;
            }
        }

        if (json.hasKey("entities")) {
            if (!parseEntities(json["entities"])) {
                std::cerr << "Failed to parse entities" << std::endl;
                return false;
            }
        }

        if (json.hasKey("lights")) {
            if (!parseLights(json["lights"])) {
                std::cerr << "Failed to parse lights" << std::endl;
                return false;
            }
        }

        this->filename = filename;
        loaded = true;
        
        // Analyze surface types after loading
        analyzeSurfaceTypes();
        
        std::cout << "Successfully loaded map: " << filename << std::endl;
        std::cout << "  Brushes: " << brushes.size() << std::endl;
        std::cout << "  Entities: " << entities.size() << std::endl;
        std::cout << "  Lights: " << lights.size() << std::endl;
        
        // Report surface type distribution
        auto floors = getFloorBrushes();
        auto ceilings = getCeilingBrushes();
        auto walls = getWallBrushes();
        std::cout << "  Floor surfaces: " << floors.size() << std::endl;
        std::cout << "  Ceiling surfaces: " << ceilings.size() << std::endl;
        std::cout << "  Wall surfaces: " << walls.size() << std::endl;
        
        return validate();
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return false;
    }
}

bool Map::saveToFile(const std::string& filename) const {
    SimpleJson json;
    json["version"] = SimpleJson("1.0");
    json["worldSettings"] = worldSettingsToJson();
    json["geometry"] = brushesToJson();
    json["entities"] = entitiesToJson();
    json["lights"] = lightsToJson();

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to create map file: " << filename << std::endl;
        return false;
    }

    file << json.toString();
    file.close();
    
    std::cout << "Map saved to: " << filename << std::endl;
    return true;
}

void Map::clear() {
    worldSettings = WorldSettings();
    brushes.clear();
    entities.clear();
    lights.clear();
    filename.clear();
    loaded = false;
}

Entity* Map::getPlayerStart() {
    for (auto& entity : entities) {
        if (entity.type == EntityType::PLAYER_START) {
            return &entity;
        }
    }
    return nullptr;
}

std::vector<Entity*> Map::getEntitiesByType(EntityType type) {
    std::vector<Entity*> result;
    for (auto& entity : entities) {
        if (entity.type == type) {
            result.push_back(&entity);
        }
    }
    return result;
}

void Map::removeBrush(uint32_t id) {
    brushes.erase(
        std::remove_if(brushes.begin(), brushes.end(),
                      [id](const Brush& b) { return b.id == id; }),
        brushes.end());
}

void Map::removeEntity(size_t index) {
    if (index < entities.size()) {
        entities.erase(entities.begin() + index);
    }
}

void Map::removeLight(size_t index) {
    if (index < lights.size()) {
        lights.erase(lights.begin() + index);
    }
}

bool Map::validate() const {
    // Check for player start
    bool hasPlayerStart = false;
    for (const auto& entity : entities) {
        if (entity.type == EntityType::PLAYER_START) {
            hasPlayerStart = true;
            break;
        }
    }
    
    if (!hasPlayerStart) {
        std::cerr << "Warning: Map has no player start position" << std::endl;
    }
    
    // Check brush validity
    for (const auto& brush : brushes) {
        if (brush.vertices.empty()) {
            std::cerr << "Error: Brush " << brush.id << " has no vertices" << std::endl;
            return false;
        }
        if (brush.faces.empty()) {
            std::cerr << "Error: Brush " << brush.id << " has no faces" << std::endl;
            return false;
        }
        
        // Check face indices
        for (uint32_t face_idx : brush.faces) {
            if (face_idx >= brush.vertices.size()) {
                std::cerr << "Error: Brush " << brush.id << " has invalid face index: " 
                         << face_idx << std::endl;
                return false;
            }
        }
    }
    
    return true;
}

bool Map::parseWorldSettings(const SimpleJson& json) {
    if (!json.isObject()) return false;
    
    worldSettings.gravity = json.getNumber("gravity", worldSettings.gravity);
    
    if (json.hasKey("ambientLight") && json["ambientLight"].isArray() && json["ambientLight"].size() >= 3) {
        worldSettings.ambientLight.x = json["ambientLight"][0].getNumber();
        worldSettings.ambientLight.y = json["ambientLight"][1].getNumber();
        worldSettings.ambientLight.z = json["ambientLight"][2].getNumber();
    }
    
    if (json.hasKey("backgroundColor") && json["backgroundColor"].isArray() && json["backgroundColor"].size() >= 3) {
        worldSettings.backgroundColor.x = json["backgroundColor"][0].getNumber();
        worldSettings.backgroundColor.y = json["backgroundColor"][1].getNumber();
        worldSettings.backgroundColor.z = json["backgroundColor"][2].getNumber();
    }
    
    return true;
}

bool Map::parseBrushes(const SimpleJson& json) {
    if (!json.isObject() || !json.hasKey("brushes")) return true; // No brushes is OK
    
    const SimpleJson& brushArray = json["brushes"];
    if (!brushArray.isArray()) return false;
    
    for (size_t i = 0; i < brushArray.size(); ++i) {
        const SimpleJson& brushJson = brushArray[i];
        if (!brushJson.isObject()) continue;
        
        Brush brush;
        brush.id = static_cast<uint32_t>(brushJson.getNumber("id", i));
        brush.material = brushJson.getString("material", "default");
        
        // Parse color if present
        if (brushJson.hasKey("color") && brushJson["color"].isArray() && brushJson["color"].size() >= 3) {
            brush.color.x = brushJson["color"][0].getNumber();
            brush.color.y = brushJson["color"][1].getNumber();
            brush.color.z = brushJson["color"][2].getNumber();
        }
        
        // Parse texture if present
        brush.texture = brushJson.getString("texture", "");
        
        // Parse surface type if present
        std::string typeStr = brushJson.getString("type", "");
        if (typeStr == "floor") {
            brush.surfaceType = SurfaceType::FLOOR;
        } else if (typeStr == "ceiling") {
            brush.surfaceType = SurfaceType::CEILING;
        } else if (typeStr == "wall") {
            brush.surfaceType = SurfaceType::WALL;
        } else {
            brush.surfaceType = SurfaceType::UNKNOWN;
        }
        
        // Parse vertices
        if (brushJson.hasKey("vertices") && brushJson["vertices"].isArray()) {
            const SimpleJson& vertArray = brushJson["vertices"];
            for (size_t v = 0; v < vertArray.size(); v += 3) {
                if (v + 2 < vertArray.size()) {
                    glm::vec3 vertex(
                        vertArray[v].getNumber(),
                        vertArray[v + 1].getNumber(),
                        vertArray[v + 2].getNumber()
                    );
                    brush.vertices.push_back(vertex);
                }
            }
        }
        
        // Parse texture coordinates
        if (brushJson.hasKey("texCoords") && brushJson["texCoords"].isArray()) {
            const SimpleJson& texArray = brushJson["texCoords"];
            for (size_t t = 0; t < texArray.size(); t += 2) {
                if (t + 1 < texArray.size()) {
                    glm::vec2 texCoord(
                        texArray[t].getNumber(),
                        texArray[t + 1].getNumber()
                    );
                    brush.texCoords.push_back(texCoord);
                }
            }
        }
        
        // Parse faces
        if (brushJson.hasKey("faces") && brushJson["faces"].isArray()) {
            const SimpleJson& faceArray = brushJson["faces"];
            for (size_t f = 0; f < faceArray.size(); ++f) {
                brush.faces.push_back(static_cast<uint32_t>(faceArray[f].getNumber()));
            }
        }
        
        brushes.push_back(brush);
    }
    
    return true;
}

bool Map::parseEntities(const SimpleJson& json) {
    if (!json.isArray()) return false;
    
    for (size_t i = 0; i < json.size(); ++i) {
        const SimpleJson& entityJson = json[i];
        if (!entityJson.isObject()) continue;
        
        Entity entity;
        
        // Parse type
        std::string typeStr = entityJson.getString("type", "player_start");
        if (typeStr == "player_start") entity.type = EntityType::PLAYER_START;
        else if (typeStr == "light") entity.type = EntityType::LIGHT;
        else if (typeStr == "enemy_spawn") entity.type = EntityType::ENEMY_SPAWN;
        else if (typeStr == "item_pickup") entity.type = EntityType::ITEM_PICKUP;
        else if (typeStr == "trigger") entity.type = EntityType::TRIGGER;
        else entity.type = EntityType::PLAYER_START;
        
        entity.name = entityJson.getString("name", "");
        
        // Parse position
        if (entityJson.hasKey("position") && entityJson["position"].isArray() && entityJson["position"].size() >= 3) {
            entity.position.x = entityJson["position"][0].getNumber();
            entity.position.y = entityJson["position"][1].getNumber();
            entity.position.z = entityJson["position"][2].getNumber();
        }
        
        // Parse rotation
        if (entityJson.hasKey("rotation") && entityJson["rotation"].isArray() && entityJson["rotation"].size() >= 3) {
            entity.rotation.x = entityJson["rotation"][0].getNumber();
            entity.rotation.y = entityJson["rotation"][1].getNumber();
            entity.rotation.z = entityJson["rotation"][2].getNumber();
        }
        
        entities.push_back(entity);
    }
    
    return true;
}

bool Map::parseLights(const SimpleJson& json) {
    if (!json.isArray()) return false;
    
    for (size_t i = 0; i < json.size(); ++i) {
        const SimpleJson& lightJson = json[i];
        if (!lightJson.isObject()) continue;
        
        Light light;
        
        // Parse type
        std::string typeStr = lightJson.getString("type", "point");
        if (typeStr == "directional") light.type = LightType::DIRECTIONAL;
        else if (typeStr == "point") light.type = LightType::POINT;
        else if (typeStr == "spot") light.type = LightType::SPOT;
        else light.type = LightType::POINT;
        
        // Parse position
        if (lightJson.hasKey("position") && lightJson["position"].isArray() && lightJson["position"].size() >= 3) {
            light.position.x = lightJson["position"][0].getNumber();
            light.position.y = lightJson["position"][1].getNumber();
            light.position.z = lightJson["position"][2].getNumber();
        }
        
        // Parse color
        if (lightJson.hasKey("color") && lightJson["color"].isArray() && lightJson["color"].size() >= 3) {
            light.color.x = lightJson["color"][0].getNumber();
            light.color.y = lightJson["color"][1].getNumber();
            light.color.z = lightJson["color"][2].getNumber();
        }
        
        light.intensity = lightJson.getNumber("intensity", light.intensity);
        light.range = lightJson.getNumber("range", light.range);
        
        lights.push_back(light);
    }
    
    return true;
}

SimpleJson Map::worldSettingsToJson() const {
    SimpleJson json;
    json["gravity"] = SimpleJson(worldSettings.gravity);
    
    SimpleJson ambientLight;
    ambientLight.push_back(SimpleJson(worldSettings.ambientLight.x));
    ambientLight.push_back(SimpleJson(worldSettings.ambientLight.y));
    ambientLight.push_back(SimpleJson(worldSettings.ambientLight.z));
    json["ambientLight"] = ambientLight;
    
    SimpleJson backgroundColor;
    backgroundColor.push_back(SimpleJson(worldSettings.backgroundColor.x));
    backgroundColor.push_back(SimpleJson(worldSettings.backgroundColor.y));
    backgroundColor.push_back(SimpleJson(worldSettings.backgroundColor.z));
    json["backgroundColor"] = backgroundColor;
    
    return json;
}

SimpleJson Map::brushesToJson() const {
    SimpleJson geometry;
    SimpleJson brushArray;
    
    for (const auto& brush : brushes) {
        SimpleJson brushJson;
        brushJson["id"] = SimpleJson(static_cast<double>(brush.id));
        brushJson["material"] = SimpleJson(brush.material);
        
        SimpleJson color;
        color.push_back(SimpleJson(brush.color.x));
        color.push_back(SimpleJson(brush.color.y));
        color.push_back(SimpleJson(brush.color.z));
        brushJson["color"] = color;
        
        if (!brush.texture.empty()) {
            brushJson["texture"] = SimpleJson(brush.texture);
        }
        
        // Save surface type
        switch (brush.surfaceType) {
            case SurfaceType::FLOOR:
                brushJson["type"] = SimpleJson("floor");
                break;
            case SurfaceType::CEILING:
                brushJson["type"] = SimpleJson("ceiling");
                break;
            case SurfaceType::WALL:
                brushJson["type"] = SimpleJson("wall");
                break;
            default:
                // Don't save type for UNKNOWN
                break;
        }
        
        SimpleJson vertices;
        for (const auto& vertex : brush.vertices) {
            vertices.push_back(SimpleJson(vertex.x));
            vertices.push_back(SimpleJson(vertex.y));
            vertices.push_back(SimpleJson(vertex.z));
        }
        brushJson["vertices"] = vertices;
        
        if (!brush.texCoords.empty()) {
            SimpleJson texCoords;
            for (const auto& texCoord : brush.texCoords) {
                texCoords.push_back(SimpleJson(texCoord.x));
                texCoords.push_back(SimpleJson(texCoord.y));
            }
            brushJson["texCoords"] = texCoords;
        }
        
        SimpleJson faces;
        for (uint32_t face_idx : brush.faces) {
            faces.push_back(SimpleJson(static_cast<double>(face_idx)));
        }
        brushJson["faces"] = faces;
        
        brushArray.push_back(brushJson);
    }
    
    geometry["brushes"] = brushArray;
    return geometry;
}

SimpleJson Map::entitiesToJson() const {
    SimpleJson entitiesJson;
    
    for (const auto& entity : entities) {
        SimpleJson entityJson;
        
        // Type
        switch (entity.type) {
            case EntityType::PLAYER_START: entityJson["type"] = SimpleJson("player_start"); break;
            case EntityType::LIGHT: entityJson["type"] = SimpleJson("light"); break;
            case EntityType::ENEMY_SPAWN: entityJson["type"] = SimpleJson("enemy_spawn"); break;
            case EntityType::ITEM_PICKUP: entityJson["type"] = SimpleJson("item_pickup"); break;
            case EntityType::TRIGGER: entityJson["type"] = SimpleJson("trigger"); break;
        }
        
        entityJson["name"] = SimpleJson(entity.name);
        
        // Position
        SimpleJson position;
        position.push_back(SimpleJson(entity.position.x));
        position.push_back(SimpleJson(entity.position.y));
        position.push_back(SimpleJson(entity.position.z));
        entityJson["position"] = position;
        
        // Rotation
        SimpleJson rotation;
        rotation.push_back(SimpleJson(entity.rotation.x));
        rotation.push_back(SimpleJson(entity.rotation.y));
        rotation.push_back(SimpleJson(entity.rotation.z));
        entityJson["rotation"] = rotation;
        
        entitiesJson.push_back(entityJson);
    }
    
    return entitiesJson;
}

SimpleJson Map::lightsToJson() const {
    SimpleJson lightsJson;
    
    for (const auto& light : lights) {
        SimpleJson lightJson;
        
        // Type
        switch (light.type) {
            case LightType::DIRECTIONAL: lightJson["type"] = SimpleJson("directional"); break;
            case LightType::POINT: lightJson["type"] = SimpleJson("point"); break;
            case LightType::SPOT: lightJson["type"] = SimpleJson("spot"); break;
        }
        
        // Position
        SimpleJson position;
        position.push_back(SimpleJson(light.position.x));
        position.push_back(SimpleJson(light.position.y));
        position.push_back(SimpleJson(light.position.z));
        lightJson["position"] = position;
        
        // Color
        SimpleJson color;
        color.push_back(SimpleJson(light.color.x));
        color.push_back(SimpleJson(light.color.y));
        color.push_back(SimpleJson(light.color.z));
        lightJson["color"] = color;
        
        lightJson["intensity"] = SimpleJson(light.intensity);
        lightJson["range"] = SimpleJson(light.range);
        
        lightsJson.push_back(lightJson);
    }
    
    return lightsJson;
}

std::vector<const Brush*> Map::getFloorBrushes() const {
    return getBrushesByType(SurfaceType::FLOOR);
}

std::vector<const Brush*> Map::getCeilingBrushes() const {
    return getBrushesByType(SurfaceType::CEILING);
}

std::vector<const Brush*> Map::getWallBrushes() const {
    return getBrushesByType(SurfaceType::WALL);
}

std::vector<const Brush*> Map::getBrushesByType(SurfaceType type) const {
    std::vector<const Brush*> result;
    for (const auto& brush : brushes) {
        if (brush.surfaceType == type) {
            result.push_back(&brush);
        }
    }
    return result;
}

void Map::analyzeSurfaceTypes() {
    for (auto& brush : brushes) {
        // Only automatically determine surface type if not explicitly set
        if (brush.surfaceType == SurfaceType::UNKNOWN) {
            brush.surfaceType = determineSurfaceType(brush);
        }
    }
}

SurfaceType Map::determineSurfaceType(const Brush& brush) {
    if (brush.vertices.size() < 3 || brush.faces.size() < 3) {
        return SurfaceType::UNKNOWN;
    }
    
    // Calculate the average normal of all faces in this brush
    glm::vec3 avgNormal(0.0f);
    int validFaces = 0;
    
    for (size_t i = 0; i < brush.faces.size(); i += 3) {
        if (i + 2 < brush.faces.size()) {
            uint32_t idx0 = brush.faces[i];
            uint32_t idx1 = brush.faces[i + 1];
            uint32_t idx2 = brush.faces[i + 2];
            
            if (idx0 < brush.vertices.size() && idx1 < brush.vertices.size() && idx2 < brush.vertices.size()) {
                glm::vec3 normal = calculateFaceNormal(
                    brush.vertices[idx0],
                    brush.vertices[idx1],
                    brush.vertices[idx2]
                );
                
                // Only count valid normals
                if (glm::length(normal) > 0.001f) {
                    avgNormal += normal;
                    validFaces++;
                }
            }
        }
    }
    
    if (validFaces == 0) {
        return SurfaceType::UNKNOWN;
    }
    
    avgNormal = glm::normalize(avgNormal);
    
    // Determine surface type based on normal direction
    // Floor: normal points mostly upward (Y+)
    // Ceiling: normal points mostly downward (Y-)
    // Wall: normal is mostly horizontal
    
    float upwardDot = glm::dot(avgNormal, glm::vec3(0.0f, 1.0f, 0.0f));
    float downwardDot = glm::dot(avgNormal, glm::vec3(0.0f, -1.0f, 0.0f));
    
    const float threshold = 0.7f; // cos(45 degrees) approximately
    
    if (upwardDot > threshold) {
        return SurfaceType::FLOOR;
    } else if (downwardDot > threshold) {
        return SurfaceType::CEILING;
    } else {
        return SurfaceType::WALL;
    }
}

glm::vec3 Map::calculateFaceNormal(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2) {
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 normal = glm::cross(edge1, edge2);
    
    // Return normalized normal, or zero vector if degenerate
    float length = glm::length(normal);
    if (length > 0.001f) {
        return normal / length;
    }
    return glm::vec3(0.0f);
}

} // namespace silic2