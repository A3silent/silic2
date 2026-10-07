#include "engine/map.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>

namespace silic2 {
namespace {
using J = SimpleJson;
[[noreturn]] void fail(const std::string &path, const std::string &reason) {
    throw std::runtime_error(path + ": " + reason);
}
void object(const J &j, const std::string &p) {
    if (!j.isObject())
        fail(p, "expected object");
}
void array(const J &j, const std::string &p) {
    if (!j.isArray())
        fail(p, "expected array");
    if (j.size() > 200000)
        fail(p, "array exceeds 200000 values");
}
double number(const J &j, const std::string &p) {
    if (!j.isNumber() || !std::isfinite(j.getNumber()) ||
        std::abs(j.getNumber()) > std::numeric_limits<float>::max())
        fail(p, "expected finite number in float range");
    return j.getNumber();
}
double num(const J &j, const char *k, double d, const std::string &p) {
    return j.hasKey(k) ? number(j[k], p + "." + k) : d;
}
int64_t integer(const J &j, const std::string &p, int64_t lo, int64_t hi) {
    double n = number(j, p);
    if (n != std::floor(n) || n < lo || n > hi)
        fail(p, "integer out of range");
    return static_cast<int64_t>(n);
}
std::string str(const J &j, const char *k, const std::string &d, const std::string &p) {
    if (!j.hasKey(k))
        return d;
    if (!j[k].isString())
        fail(p + "." + k, "expected string");
    return j[k].getString();
}
glm::vec3 vector(const J &j, const std::string &p) {
    array(j, p);
    if (j.size() != 3)
        fail(p, "expected exactly three components");
    return {number(j[0], p + "[0]"), number(j[1], p + "[1]"), number(j[2], p + "[2]")};
}
glm::vec3 vec(const J &j, const char *k, glm::vec3 d, const std::string &p) {
    return j.hasKey(k) ? vector(j[k], p + "." + k) : d;
}
J jvec(glm::vec3 v) {
    J a = J::array();
    for (int i = 0; i < 3; ++i)
        a.push_back(J(double(v[i])));
    return a;
}
J original(const J &j) { return j.isObject() ? j : J::object(); }
SurfaceType surface(const J &j, const std::string &p) {
    auto s = str(j, "type", "", p);
    if (s.empty())
        return SurfaceType::UNKNOWN;
    if (s == "floor")
        return SurfaceType::FLOOR;
    if (s == "wall")
        return SurfaceType::WALL;
    if (s == "ceiling")
        return SurfaceType::CEILING;
    fail(p + ".type", "unknown surface type " + s);
}
void appearance(const J &j, Brush &b, const std::string &p) {
    b.material = str(j, "material", "default", p);
    b.texture = str(j, "texture", "", p);
    b.color = vec(j, "color", glm::vec3(.8f), p);
    b.surfaceType = surface(j, p);
}
void appearance(J &j, const std::string &material, const std::string &texture, glm::vec3 color,
                SurfaceType type) {
    j["material"] = J(material);
    j["texture"] = J(texture);
    j["color"] = jvec(color);
    if (type == SurfaceType::UNKNOWN)
        j.erase("type");
    else
        j["type"] = J(type == SurfaceType::FLOOR     ? "floor"
                      : type == SurfaceType::CEILING ? "ceiling"
                                                     : "wall");
}
void spatial(glm::vec3 v, const std::string &path) {
    for (int i = 0; i < 3; ++i)
        if (!std::isfinite(v[i]) || std::abs(v[i]) > 100000.f)
            fail(path, "spatial components must be finite and within +/-100000 world units");
}
MapObstacle bounds(const Brush &b) {
    MapObstacle r;
    r.brushId = b.id;
    r.min = r.max = b.vertices.front();
    for (auto v : b.vertices) {
        r.min = glm::min(r.min, v);
        r.max = glm::max(r.max, v);
    }
    return r;
}
const char *entityName(EntityType t) {
    switch (t) {
    case EntityType::PLAYER_START:
        return "player_start";
    case EntityType::LIGHT:
        return "light";
    case EntityType::ENEMY_SPAWN:
        return "enemy_spawn";
    case EntityType::ITEM_PICKUP:
        return "item_pickup";
    case EntityType::TRIGGER:
        return "trigger";
    case EntityType::RIFT:
        return "rift";
    case EntityType::SENTRY_SPAWN:
        return "sentry_spawn";
    }
    fail("entity.type", "invalid enum");
}
EntityType entityType(const std::string &s, const std::string &p) {
    for (auto t :
         {EntityType::PLAYER_START, EntityType::LIGHT, EntityType::ENEMY_SPAWN,
          EntityType::ITEM_PICKUP, EntityType::TRIGGER, EntityType::RIFT, EntityType::SENTRY_SPAWN})
        if (s == entityName(t))
            return t;
    fail(p, "unknown entity type " + s);
}
Brush compile(const GeometryObject &o) {
    Brush b;
    b.id = o.id;
    b.material = o.material;
    b.texture = o.texture;
    b.color = o.color;
    b.surfaceType = o.surfaceType;
    const auto a = o.position - o.size * .5f, c = o.position + o.size * .5f;
    if (o.shape == ObjectShape::Plane) {
        b.vertices = {{a.x, a.y, a.z}, {c.x, a.y, a.z}, {c.x, a.y, c.z}, {a.x, a.y, c.z}};
        b.faces = {0, 2, 1, 0, 3, 2};
        b.texCoords = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        if (o.surfaceType == SurfaceType::CEILING)
            for (size_t i = 0; i < b.faces.size(); i += 3)
                std::swap(b.faces[i + 1], b.faces[i + 2]);
    } else {
        b.vertices = {{a.x, a.y, a.z}, {c.x, a.y, a.z}, {c.x, c.y, a.z}, {a.x, c.y, a.z},
                      {a.x, a.y, c.z}, {c.x, a.y, c.z}, {c.x, c.y, c.z}, {a.x, c.y, c.z}};
        b.faces = {0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 4, 7, 0, 7, 3,
                   1, 2, 6, 1, 6, 5, 3, 7, 6, 3, 6, 2, 0, 1, 5, 0, 5, 4};
    }
    return b;
}
// Only the published v1 sample is upgraded. Materials never select gameplay.
J migrate(J j) {
    if (j.hasKey("schemaVersion") || !j.hasKey("contentVersion") ||
        !j["contentVersion"].isString() ||
        j["contentVersion"].getString() != "starry-crystal-arena-v1")
        return j;
    array(j["entities"], "entities");
    const char *names[] = {"rift_north", "rift_east", "rift_south", "rift_west",
                           "sentry_0",   "sentry_1",  "sentry_2"};
    for (int n = 0; n < 7; ++n) {
        int count = 0;
        for (size_t i = 0; i < j["entities"].size(); ++i) {
            auto &e = j["entities"][i];
            if (str(e, "name", "", "entities") == names[n]) {
                ++count;
                if (str(e, "type", "", "entities") != "trigger")
                    fail("legacy migration", std::string(names[n]) + " must be a trigger");
                e["type"] = J(n < 4 ? "rift" : "sentry_spawn");
                e["slot"] = J(double(n < 4 ? n : n - 4));
                if (n == 1 || n == 3)
                    e["rotation"] = jvec({0, 90, 0});
            }
        }
        if (count != 1)
            fail("legacy migration", std::string("requires one named anchor ") + names[n]);
    }
    j["schemaVersion"] = J(2.0);
    j["gameMode"] = J("arena_survival");
    j["environmentPreset"] = J("starry_crystal");
    j["arena"] = J::object();
    j["arena"]["floorBrushId"] = J(1.0);
    return j;
}
} // namespace
Map::Map() = default;
void Map::clear() { *this = Map(); }
bool Map::loadFromFile(const std::string &path) {
    try {
        std::ifstream f(path, std::ios::binary);
        if (!f)
            fail(path, "cannot open file");
        std::ostringstream b;
        b << f.rdbuf();
        if (f.bad())
            fail(path, "read failed");
        if (!loadFromJson(J::parse(b.str())))
            return false;
        filename = path;
        return true;
    } catch (const std::exception &e) {
        lastError = e.what();
        return false;
    }
}
bool Map::loadFromJson(const J &document) {
    try {
        Map candidate;
        candidate.parseDocument(migrate(document));
        candidate.rebuild();
        candidate.filename = filename;
        *this = std::move(candidate);
        return true;
    } catch (const std::exception &e) {
        lastError = e.what();
        return false;
    }
}
void Map::parseDocument(const J &d) {
    object(d, "map");
    sourceDocument = d;
    if (d.hasKey("schemaVersion"))
        schemaVersion = int(integer(d["schemaVersion"], "schemaVersion", 1, 2));
    if (d.hasKey("version") && str(d, "version", "", "map") != "1.0")
        fail("version", "unsupported legacy version");
    if (schemaVersion == 2 && (!d.hasKey("gameMode") || !d.hasKey("environmentPreset")))
        fail("schemaVersion 2", "gameMode and environmentPreset are required");
    auto mode = str(d, "gameMode", "legacy", "map");
    if (mode == "arena_survival")
        gameMode = GameMode::ArenaSurvival;
    else if (mode != "legacy")
        fail("gameMode", "unsupported value " + mode);
    auto preset = str(d, "environmentPreset", "none", "map");
    if (preset == "starry_crystal")
        environment.preset = EnvironmentPreset::StarryCrystal;
    else if (preset != "none")
        fail("environmentPreset", "unsupported value " + preset);
    contentVersion = str(d, "contentVersion", "legacy", "map");
    if (d.hasKey("worldSettings")) {
        const auto &w = d["worldSettings"];
        object(w, "worldSettings");
        worldSettings.gravity = num(w, "gravity", -9.8, "worldSettings");
        worldSettings.ambientLight =
            vec(w, "ambientLight", worldSettings.ambientLight, "worldSettings");
        worldSettings.backgroundColor =
            vec(w, "backgroundColor", worldSettings.backgroundColor, "worldSettings");
    }
    if (d.hasKey("environment")) {
        const auto &e = d["environment"];
        object(e, "environment");
        environment.transformationStart = num(e, "transformationStart", 45, "environment");
        environment.transformationDuration = num(e, "transformationDuration", 4, "environment");
        if (e.hasKey("particleCount"))
            environment.particleCount =
                integer(e["particleCount"], "environment.particleCount", 0, 2000);
    }
    if (environment.transformationStart < 0 || environment.transformationDuration <= 0)
        fail("environment", "start must be nonnegative and duration positive");
    if (d.hasKey("arena")) {
        object(d["arena"], "arena");
        if (d["arena"].hasKey("floorBrushId"))
            floorBrushId = integer(d["arena"]["floorBrushId"], "arena.floorBrushId", 0, UINT32_MAX);
    }
    if (d.hasKey("geometry")) {
        const auto &g = d["geometry"];
        object(g, "geometry");
        if (g.hasKey("brushes")) {
            array(g["brushes"], "geometry.brushes");
            for (size_t i = 0; i < g["brushes"].size(); ++i) {
                const auto &j = g["brushes"][i];
                const auto p = "geometry.brushes[" + std::to_string(i) + "]";
                object(j, p);
                Brush b;
                b.source = j;
                b.id = j.hasKey("id") ? integer(j["id"], p + ".id", 0, UINT32_MAX) : i;
                appearance(j, b, p);
                array(j["vertices"], p + ".vertices");
                if (j["vertices"].size() < 9 || j["vertices"].size() % 3)
                    fail(p + ".vertices", "expected at least three XYZ vertices");
                for (size_t n = 0; n < j["vertices"].size(); n += 3)
                    b.vertices.push_back({number(j["vertices"][n], p + ".vertices"),
                                          number(j["vertices"][n + 1], p + ".vertices"),
                                          number(j["vertices"][n + 2], p + ".vertices")});
                array(j["faces"], p + ".faces");
                if (j["faces"].size() < 3 || j["faces"].size() % 3)
                    fail(p + ".faces", "expected triangle index triples");
                for (size_t n = 0; n < j["faces"].size(); ++n)
                    b.faces.push_back(
                        integer(j["faces"][n], p + ".faces", 0, b.vertices.size() - 1));
                if (j.hasKey("texCoords")) {
                    array(j["texCoords"], p + ".texCoords");
                    if (j["texCoords"].size() != 0 &&
                        j["texCoords"].size() != b.vertices.size() * 2)
                        fail(p + ".texCoords", "expected one UV pair per vertex");
                    for (size_t n = 0; n < j["texCoords"].size(); n += 2)
                        b.texCoords.push_back({number(j["texCoords"][n], p + ".texCoords"),
                                               number(j["texCoords"][n + 1], p + ".texCoords")});
                }
                legacyBrushes.push_back(std::move(b));
            }
        }
        if (g.hasKey("objects")) {
            array(g["objects"], "geometry.objects");
            for (size_t i = 0; i < g["objects"].size(); ++i) {
                const auto &j = g["objects"][i];
                const auto p = "geometry.objects[" + std::to_string(i) + "]";
                object(j, p);
                GeometryObject o;
                o.source = j;
                o.id = integer(j["id"], p + ".id", 0, UINT32_MAX);
                o.name = str(j, "name", "", p);
                auto s = str(j, "shape", "", p);
                if (s == "box")
                    o.shape = ObjectShape::Box;
                else if (s == "plane")
                    o.shape = ObjectShape::Plane;
                else
                    fail(p + ".shape", "expected box or plane");
                o.position = vector(j["position"], p + ".position");
                o.size = vector(j["size"], p + ".size");
                if (o.size.x <= 0 || o.size.z <= 0 ||
                    (o.shape == ObjectShape::Box ? o.size.y <= 0 : o.size.y != 0))
                    fail(p + ".size",
                         "box requires positive XYZ; XZ plane requires positive X/Z and zero Y");
                if (j.hasKey("rotation"))
                    fail(p + ".rotation", "box/plane rotation is not supported");
                Brush b;
                appearance(j, b, p);
                o.material = b.material;
                o.texture = b.texture;
                o.color = b.color;
                o.surfaceType = b.surfaceType;
                objects.push_back(std::move(o));
            }
        }
    }
    if (d.hasKey("entities")) {
        array(d["entities"], "entities");
        for (size_t i = 0; i < d["entities"].size(); ++i) {
            const auto &j = d["entities"][i];
            const auto p = "entities[" + std::to_string(i) + "]";
            object(j, p);
            Entity e;
            e.source = j;
            e.type = entityType(str(j, "type", "player_start", p), p + ".type");
            e.name = str(j, "name", "", p);
            e.position = vec(j, "position", glm::vec3(0), p);
            e.rotation = vec(j, "rotation", glm::vec3(0), p);
            e.scale = vec(j, "scale", glm::vec3(1), p);
            if (e.scale.x <= 0 || e.scale.y <= 0 || e.scale.z <= 0)
                fail(p + ".scale", "components must be positive");
            e.stringProperty = str(j, "stringProperty", "", p);
            e.vec3Property = vec(j, "vec3Property", glm::vec3(0), p);
            e.floatProperty1 = num(j, "floatProperty1", 0, p);
            e.floatProperty2 = num(j, "floatProperty2", 0, p);
            if (j.hasKey("intProperty"))
                e.intProperty = integer(j["intProperty"], p + ".intProperty", INT32_MIN, INT32_MAX);
            if (j.hasKey("slot"))
                e.slot = integer(j["slot"], p + ".slot", -1, 3);
            entities.push_back(std::move(e));
        }
    }
    if (d.hasKey("lights")) {
        array(d["lights"], "lights");
        for (size_t i = 0; i < d["lights"].size(); ++i) {
            const auto &j = d["lights"][i];
            const auto p = "lights[" + std::to_string(i) + "]";
            object(j, p);
            Light l;
            l.source = j;
            auto s = str(j, "type", "point", p);
            if (s == "point")
                l.type = LightType::POINT;
            else if (s == "spot")
                l.type = LightType::SPOT;
            else if (s == "directional")
                l.type = LightType::DIRECTIONAL;
            else
                fail(p + ".type", "unknown light type");
            l.position = vec(j, "position", l.position, p);
            l.direction = vec(j, "direction", l.direction, p);
            l.color = vec(j, "color", l.color, p);
            l.intensity = num(j, "intensity", 1, p);
            l.range = num(j, "range", 10, p);
            l.innerConeAngle = num(j, "innerConeAngle", 30, p);
            l.outerConeAngle = num(j, "outerConeAngle", 45, p);
            if (l.intensity < 0 || l.range <= 0 || l.innerConeAngle < 0 ||
                l.outerConeAngle < l.innerConeAngle || l.outerConeAngle > 180)
                fail(p, "invalid intensity, range or cone angles");
            if (glm::dot(l.direction, l.direction) < 1e-12f)
                fail(p + ".direction", "must not be zero");
            if (j.hasKey("anchor")) {
                const auto &a = j["anchor"];
                object(a, p + ".anchor");
                if (a.hasKey("brushId") == a.hasKey("entityName"))
                    fail(p + ".anchor", "requires exactly one brushId or entityName");
                if (a.hasKey("brushId"))
                    l.anchorBrushId = integer(a["brushId"], p + ".anchor.brushId", 0, UINT32_MAX);
                else {
                    l.anchorEntityName = str(a, "entityName", "", p + ".anchor");
                    if (l.anchorEntityName.empty())
                        fail(p + ".anchor.entityName", "must not be empty");
                }
                l.anchorOffset = vec(a, "offset", glm::vec3(0), p + ".anchor");
            }
            sourceLights.push_back(std::move(l));
        }
    }
}
void Map::rebuild() {
    if (legacyBrushes.size() + objects.size() > 4096 || entities.size() > 8192 ||
        sourceLights.size() > 1024)
        fail("map", "exceeds limits: 4096 geometry records, 8192 entities, 1024 lights");
    for (const auto &b : legacyBrushes)
        for (const auto &v : b.vertices)
            spatial(v, "geometry.vertices");
    for (const auto &o : objects) {
        spatial(o.position, "geometry.position");
        spatial(o.size, "geometry.size");
        spatial(o.position - o.size * .5f, "geometry.minimum");
        spatial(o.position + o.size * .5f, "geometry.maximum");
    }
    for (const auto &e : entities) {
        spatial(e.position, "entities.position");
        spatial(e.scale, "entities.scale");
    }
    for (const auto &l : sourceLights) {
        spatial(l.position, "lights.position");
        spatial(l.anchorOffset, "lights.anchor.offset");
    }
    brushes = legacyBrushes;
    for (const auto &o : objects)
        brushes.push_back(compile(o));
    std::set<uint32_t> ids;
    for (auto &b : brushes) {
        for (size_t i = 0; i < b.faces.size(); i += 3) {
            const auto a = b.vertices[b.faces[i]], c = b.vertices[b.faces[i + 1]],
                       d = b.vertices[b.faces[i + 2]];
            const auto normal = glm::cross(c - a, d - a);
            if (glm::dot(normal, normal) <= 1e-16f)
                fail("geometry.brush[" + std::to_string(b.id) + "].faces", "degenerate triangle");
        }
        if (!ids.insert(b.id).second)
            fail("geometry.id", "duplicate id " + std::to_string(b.id));
        auto a = bounds(b);
        if (b.surfaceType == SurfaceType::UNKNOWN) {
            glm::vec3 normal(0);
            for (size_t i = 0; i < b.faces.size(); i += 3)
                normal += glm::cross(b.vertices[b.faces[i + 1]] - b.vertices[b.faces[i]],
                                     b.vertices[b.faces[i + 2]] - b.vertices[b.faces[i]]);
            b.surfaceType = std::abs(a.max.y - a.min.y) < .001f
                                ? (normal.y < 0 ? SurfaceType::CEILING : SurfaceType::FLOOR)
                                : SurfaceType::WALL;
        }
    }
    const Brush *floor = nullptr;
    for (const auto &b : brushes) {
        auto a = bounds(b);
        if (floorBrushId >= 0) {
            if (b.id == floorBrushId)
                floor = &b;
        } else if (!floor && b.surfaceType == SurfaceType::FLOOR &&
                   std::abs(a.max.y - a.min.y) < .001f)
            floor = &b;
    }
    if (floorBrushId >= 0 && !floor)
        fail("arena.floorBrushId", "missing geometry id " + std::to_string(floorBrushId));
    if (floor) {
        auto a = bounds(*floor);
        if (std::abs(a.max.y - a.min.y) > .001f || a.max.x <= a.min.x || a.max.z <= a.min.z ||
            floor->surfaceType != SurfaceType::FLOOR)
            fail("arena.floorBrushId", "must reference a horizontal floor");
        arenaLayout.hasFloor = true;
        arenaLayout.minXZ = {a.min.x, a.min.z};
        arenaLayout.maxXZ = {a.max.x, a.max.z};
        arenaLayout.floorY = a.max.y;
        for (const auto &b : brushes) {
            if (b.id == floor->id)
                continue;
            auto o = bounds(b);
            if (o.max.y > a.max.y + .01f && o.min.y <= a.max.y + 1.8f && o.max.x > a.min.x &&
                o.min.x < a.max.x && o.max.z > a.min.z && o.min.z < a.max.z)
                arenaLayout.obstacles.push_back(o);
        }
    }
    int playerCount = 0;
    for (const auto &e : entities) {
        if (e.type == EntityType::PLAYER_START) {
            ++playerCount;
            arenaLayout.hasPlayerStart = true;
            arenaLayout.playerStart = e.position;
        }
        if (e.type == EntityType::RIFT) {
            if (e.slot < 0 || e.slot > 3 || arenaLayout.riftActive[e.slot])
                fail("entities.rift.slot", "requires distinct slots 0..3");
            arenaLayout.riftActive[e.slot] = true;
            arenaLayout.rifts[e.slot] = e.position;
            arenaLayout.riftRotations[e.slot] = e.rotation;
            arenaLayout.riftScales[e.slot] = e.scale;
        }
        if (e.type == EntityType::SENTRY_SPAWN) {
            if (e.slot < 0 || e.slot > 2 || arenaLayout.sentryActive[e.slot])
                fail("entities.sentry_spawn.slot", "requires distinct slots 0..2");
            arenaLayout.sentryActive[e.slot] = true;
            arenaLayout.sentryPositions[e.slot] = e.position;
        }
    }
    if (gameMode == GameMode::ArenaSurvival) {
        if (arenaLayout.obstacles.size() > 64)
            fail("arena", "at most 64 navigation obstacles supported");
        if (!arenaLayout.hasFloor || floorBrushId < 0)
            fail("arena.floorBrushId", "arena_survival requires an explicit floor reference");
        if (playerCount != 1)
            fail("entities.player_start", "arena_survival requires exactly one player start");
        for (bool active : arenaLayout.riftActive)
            if (!active)
                fail("entities.rift", "arena_survival requires all four slots");
        for (bool active : arenaLayout.sentryActive)
            if (!active)
                fail("entities.sentry_spawn", "arena_survival requires all three slots");
        for (const auto &e : entities)
            if (e.type == EntityType::PLAYER_START || e.type == EntityType::RIFT ||
                e.type == EntityType::SENTRY_SPAWN) {
                float r = e.type == EntityType::SENTRY_SPAWN ? .6f : .34f;
                auto p = e.position;
                if (std::abs(p.y - arenaLayout.floorY) > .05f || p.x < arenaLayout.minXZ.x + r ||
                    p.x > arenaLayout.maxXZ.x - r || p.z < arenaLayout.minXZ.y + r ||
                    p.z > arenaLayout.maxXZ.y - r)
                    fail("entities." + e.name, "spawn must be on and inside the arena floor");
                for (const auto &o : arenaLayout.obstacles)
                    if (p.x > o.min.x - r && p.x < o.max.x + r && p.z > o.min.z - r &&
                        p.z < o.max.z + r && p.y + 1.8f > o.min.y && p.y < o.max.y)
                        fail("entities." + e.name,
                             "spawn overlaps geometry " + std::to_string(o.brushId));
            }
    }
    lights = sourceLights;
    for (auto &l : lights) {
        if (l.anchorBrushId >= 0) {
            auto it = std::find_if(brushes.begin(), brushes.end(),
                                   [&](const Brush &b) { return b.id == l.anchorBrushId; });
            if (it == brushes.end())
                fail("lights.anchor.brushId", "missing geometry reference");
            auto b = bounds(*it);
            l.position = (b.min + b.max) * .5f + l.anchorOffset;
        } else if (!l.anchorEntityName.empty()) {
            const Entity *target = nullptr;
            for (const auto &e : entities)
                if (e.name == l.anchorEntityName) {
                    if (target)
                        fail("lights.anchor.entityName", "ambiguous name " + e.name);
                    target = &e;
                }
            if (!target)
                fail("lights.anchor.entityName", "missing entity " + l.anchorEntityName);
            l.position = target->position + l.anchorOffset;
        }
        spatial(l.position, "lights.resolvedPosition");
    }
}
J Map::toJson() const {
    J d = original(sourceDocument);
    d["schemaVersion"] = J(double(schemaVersion));
    d["gameMode"] = J(gameMode == GameMode::ArenaSurvival ? "arena_survival" : "legacy");
    d["environmentPreset"] =
        J(environment.preset == EnvironmentPreset::StarryCrystal ? "starry_crystal" : "none");
    d["contentVersion"] = J(contentVersion);
    J w = original(d["worldSettings"]);
    w["gravity"] = J(double(worldSettings.gravity));
    w["ambientLight"] = jvec(worldSettings.ambientLight);
    w["backgroundColor"] = jvec(worldSettings.backgroundColor);
    d["worldSettings"] = w;
    J env = original(d["environment"]);
    env["transformationStart"] = J(double(environment.transformationStart));
    env["transformationDuration"] = J(double(environment.transformationDuration));
    env["particleCount"] = J(double(environment.particleCount));
    d["environment"] = env;
    J arena = original(d["arena"]);
    if (floorBrushId >= 0)
        arena["floorBrushId"] = J(double(floorBrushId));
    else
        arena.erase("floorBrushId");
    if (arena.size())
        d["arena"] = arena;
    else
        d.erase("arena");
    J g = original(d["geometry"]), bs = J::array(), os = J::array();
    for (const auto &b : legacyBrushes) {
        J j = original(b.source);
        j["id"] = J(double(b.id));
        appearance(j, b.material, b.texture, b.color, b.surfaceType);
        J vs = J::array(), uv = J::array(), fs = J::array();
        for (auto v : b.vertices)
            for (int k = 0; k < 3; ++k)
                vs.push_back(J(double(v[k])));
        for (auto v : b.texCoords)
            for (int k = 0; k < 2; ++k)
                uv.push_back(J(double(v[k])));
        for (auto f : b.faces)
            fs.push_back(J(double(f)));
        j["vertices"] = vs;
        j["texCoords"] = uv;
        j["faces"] = fs;
        bs.push_back(j);
    }
    for (const auto &o : objects) {
        J j = original(o.source);
        j["id"] = J(double(o.id));
        j["name"] = J(o.name);
        j["shape"] = J(o.shape == ObjectShape::Box ? "box" : "plane");
        j["position"] = jvec(o.position);
        j["size"] = jvec(o.size);
        appearance(j, o.material, o.texture, o.color, o.surfaceType);
        os.push_back(j);
    }
    g["brushes"] = bs;
    g["objects"] = os;
    d["geometry"] = g;
    J es = J::array();
    for (const auto &e : entities) {
        J j = original(e.source);
        j["type"] = J(entityName(e.type));
        j["name"] = J(e.name);
        j["position"] = jvec(e.position);
        j["rotation"] = jvec(e.rotation);
        j["scale"] = jvec(e.scale);
        j["stringProperty"] = J(e.stringProperty);
        j["vec3Property"] = jvec(e.vec3Property);
        j["floatProperty1"] = J(double(e.floatProperty1));
        j["floatProperty2"] = J(double(e.floatProperty2));
        j["intProperty"] = J(double(e.intProperty));
        if (e.slot >= 0)
            j["slot"] = J(double(e.slot));
        else
            j.erase("slot");
        es.push_back(j);
    }
    d["entities"] = es;
    J ls = J::array();
    for (const auto &l : sourceLights) {
        J j = original(l.source);
        j["type"] = J(l.type == LightType::POINT  ? "point"
                      : l.type == LightType::SPOT ? "spot"
                                                  : "directional");
        j["position"] = jvec(l.position);
        j["direction"] = jvec(l.direction);
        j["color"] = jvec(l.color);
        j["intensity"] = J(double(l.intensity));
        j["range"] = J(double(l.range));
        j["innerConeAngle"] = J(double(l.innerConeAngle));
        j["outerConeAngle"] = J(double(l.outerConeAngle));
        if (l.anchorBrushId >= 0 || !l.anchorEntityName.empty()) {
            J a = original(j["anchor"]);
            a.erase("brushId");
            a.erase("entityName");
            if (l.anchorBrushId >= 0)
                a["brushId"] = J(double(l.anchorBrushId));
            else
                a["entityName"] = J(l.anchorEntityName);
            a["offset"] = jvec(l.anchorOffset);
            j["anchor"] = a;
        } else
            j.erase("anchor");
        ls.push_back(j);
    }
    d["lights"] = ls;
    return d;
}
bool Map::validate() const {
    try {
        Map c;
        c.parseDocument(toJson());
        c.rebuild();
        lastError.clear();
        return true;
    } catch (const std::exception &e) {
        lastError = e.what();
        return false;
    }
}
bool Map::saveToFile(const std::string &path) const {
    if (!validate())
        return false;
    const std::string tmp = path + ".tmp";
    try {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f)
            fail(path, "cannot create temporary file");
        f << toJson().toString() << '\n';
        f.close();
        if (!f)
            fail(path, "write failed");
        std::filesystem::rename(tmp, path);
        lastError.clear();
        return true;
    } catch (const std::exception &e) {
        lastError = e.what();
        std::error_code ec;
        std::filesystem::remove(tmp, ec);
        return false;
    }
}
bool Map::applyCandidate(const Map &c) {
    try {
        return loadFromJson(c.toJson());
    } catch (const std::exception &e) {
        lastError = e.what();
        return false;
    }
}
bool Map::setWorldSettings(const WorldSettings &v) {
    Map c = *this;
    c.worldSettings = v;
    return applyCandidate(c);
}
bool Map::setGameMode(GameMode v) {
    Map c = *this;
    c.gameMode = v;
    c.schemaVersion = 2;
    return applyCandidate(c);
}
bool Map::setEnvironment(const EnvironmentSettings &v) {
    Map c = *this;
    c.environment = v;
    c.schemaVersion = 2;
    return applyCandidate(c);
}
#define ADD(Name, Type, Field)                                                                     \
    bool Map::add##Name(const Type &v) {                                                           \
        Map c = *this;                                                                             \
        c.Field.push_back(v);                                                                      \
        return applyCandidate(c);                                                                  \
    }
ADD(Brush, Brush, legacyBrushes)
ADD(Object, GeometryObject, objects)
ADD(Entity, Entity, entities) ADD(Light, Light, sourceLights)
#undef ADD
#define UPDATE(Name, Type, Field)                                                                  \
    bool Map::update##Name(size_t i, const Type &v) {                                              \
        if (i >= Field.size()) {                                                                   \
            lastError = #Name " index out of range";                                               \
            return false;                                                                          \
        }                                                                                          \
        Map c = *this;                                                                             \
        c.Field[i] = v;                                                                            \
        return applyCandidate(c);                                                                  \
    }
    UPDATE(Brush, Brush, legacyBrushes) UPDATE(Object, GeometryObject, objects)
        UPDATE(Entity, Entity, entities) UPDATE(Light, Light, sourceLights)
#undef UPDATE
#define REMOVE_ID(Name, Field)                                                                     \
    bool Map::remove##Name(uint32_t id) {                                                          \
        Map c = *this;                                                                             \
        auto it = std::find_if(c.Field.begin(), c.Field.end(),                                     \
                               [&](const auto &b) { return b.id == id; });                         \
        if (it == c.Field.end()) {                                                                 \
            lastError = #Name " id not found";                                                     \
            return false;                                                                          \
        }                                                                                          \
        c.Field.erase(it);                                                                         \
        return applyCandidate(c);                                                                  \
    }
            REMOVE_ID(Brush, legacyBrushes) REMOVE_ID(Object, objects)
#undef REMOVE_ID
#define REMOVE_INDEX(Name, Field)                                                                  \
    bool Map::remove##Name(size_t i) {                                                             \
        if (i >= Field.size()) {                                                                   \
            lastError = #Name " index out of range";                                               \
            return false;                                                                          \
        }                                                                                          \
        Map c = *this;                                                                             \
        c.Field.erase(c.Field.begin() + i);                                                        \
        return applyCandidate(c);                                                                  \
    }
                REMOVE_INDEX(Entity, entities) REMOVE_INDEX(Light, sourceLights)
#undef REMOVE_INDEX
                    const Entity *Map::getPlayerStart() const {
    for (const auto &e : entities)
        if (e.type == EntityType::PLAYER_START)
            return &e;
    return nullptr;
}
Entity *Map::getPlayerStart() {
    for (auto &e : entities)
        if (e.type == EntityType::PLAYER_START)
            return &e;
    return nullptr;
}
std::vector<Entity *> Map::getEntitiesByType(EntityType t) {
    std::vector<Entity *> result;
    for (auto &e : entities)
        if (e.type == t)
            result.push_back(&e);
    return result;
}
std::vector<const Brush *> Map::getBrushesByType(SurfaceType t) const {
    std::vector<const Brush *> result;
    for (const auto &b : brushes)
        if (b.surfaceType == t)
            result.push_back(&b);
    return result;
}
std::vector<const Brush *> Map::getFloorBrushes() const {
    return getBrushesByType(SurfaceType::FLOOR);
}
std::vector<const Brush *> Map::getCeilingBrushes() const {
    return getBrushesByType(SurfaceType::CEILING);
}
std::vector<const Brush *> Map::getWallBrushes() const {
    return getBrushesByType(SurfaceType::WALL);
}
} // namespace silic2
