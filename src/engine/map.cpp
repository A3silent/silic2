#include "engine/map.h"
#include "engine/mesh_asset.h"
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
    if(o.shape==ObjectShape::Ramp) {
        const auto h=o.size*.5f;
        b.vertices={{-h.x,-h.y,h.z},{h.x,-h.y,h.z},{-h.x,-h.y,-h.z},{h.x,-h.y,-h.z},{-h.x,h.y,-h.z},{h.x,h.y,-h.z}};
        b.faces={0,1,5,0,5,4,0,2,3,0,3,1,2,4,5,2,5,3,1,3,5,0,4,2};
        for(auto& v:b.vertices)v+=o.position;
    }
    if(o.rotation!=glm::vec3(0)) {
        auto transform=staticTransform(o.position,o.rotation,glm::vec3(1));
        for(auto& v:b.vertices)v=glm::vec3(transform*glm::vec4(v-o.position,1));
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
const char* Map::getDisplayName() const {
    if(gameMode==GameMode::Exploration) return "CRYSTAL OBSERVATORY";
    switch(environment.preset) {
    case EnvironmentPreset::HollowOrbit: return "HOLLOW ORBIT";
    case EnvironmentPreset::StarryCrystal: return "STARRY CRYSTAL";
    case EnvironmentPreset::None: return gameMode==GameMode::ArenaSurvival ? "ARENA" : "MAP";
    }
    return "MAP";
}
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
        if (!loadFromJson(J::parse(b.str()),std::filesystem::absolute(path).parent_path().string()))
            return false;
        filename = path;
        return true;
    } catch (const std::exception &e) {
        lastError = e.what();
        return false;
    }
}
bool Map::loadFromJson(const J &document) { return loadFromJson(document,assetBaseDirectory); }
bool Map::loadFromJson(const J &document,const std::string& baseDirectory) {
    try {
        Map candidate;
        candidate.assetBaseDirectory=std::filesystem::absolute(baseDirectory.empty()?".":baseDirectory).lexically_normal().string();
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
        schemaVersion = int(integer(d["schemaVersion"], "schemaVersion", 1, 3));
    if (d.hasKey("version") && str(d, "version", "", "map") != "1.0")
        fail("version", "unsupported legacy version");
    if (schemaVersion >= 2 && (!d.hasKey("gameMode") || !d.hasKey("environmentPreset")))
        fail("schemaVersion 2", "gameMode and environmentPreset are required");
    auto mode = str(d, "gameMode", "legacy", "map");
    if (mode == "arena_survival")
        gameMode = GameMode::ArenaSurvival;
    else if (mode == "exploration")
        gameMode = GameMode::Exploration;
    else if (mode != "legacy")
        fail("gameMode", "unsupported value " + mode);
    auto preset = str(d, "environmentPreset", "none", "map");
    if (preset == "starry_crystal")
        environment.preset = EnvironmentPreset::StarryCrystal;
    else if (preset == "hollow_orbit")
        environment.preset = EnvironmentPreset::HollowOrbit;
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
        environment.architecture=str(e,"architecture","","environment");
        environment.landmarkPosition=vec(e,"landmarkPosition",glm::vec3(0),"environment");
        environment.landmarkScale=vec(e,"landmarkScale",glm::vec3(1),"environment");
        spatial(environment.landmarkPosition,"environment.landmarkPosition");spatial(environment.landmarkScale,"environment.landmarkScale");
        environment.transformationStart = num(e, "transformationStart", 45, "environment");
        environment.transformationDuration = num(e, "transformationDuration", 4, "environment");
        if (e.hasKey("particleCount"))
            environment.particleCount =
                integer(e["particleCount"], "environment.particleCount", 0, 2000);
    }
    if (environment.transformationStart < 0 || environment.transformationDuration <= 0)
        fail("environment", "start must be nonnegative and duration positive");
    if(d.hasKey("spatial")) {
        const auto& settings=d["spatial"];object(settings,"spatial");
        if(schemaVersion!=3 || str(settings,"mode","","spatial")!="triangles")fail("spatial","requires schemaVersion 3 and mode triangles");
        spatialSettings.enabled=true;
        const auto& nav=settings["navigation"];object(nav,"spatial.navigation");array(nav["nodes"],"navigation.nodes");array(nav["links"],"navigation.links");
        if(nav["nodes"].size()>512 || nav["links"].size()>2048)fail("navigation","exceeds 512 nodes / 2048 links");
        std::set<uint32_t> ids;
        for(size_t i=0;i<nav["nodes"].size();++i) { const auto& item=nav["nodes"][i];object(item,"navigation.node");SpatialNavNode n;
            n.id=integer(item["id"],"navigation.node.id",1,UINT32_MAX);n.position=vector(item["position"],"navigation.node.position");spatial(n.position,"navigation.node.position");
            if(!ids.insert(n.id).second)fail("navigation.node.id","duplicate id");spatialSettings.nodes.push_back(n);
        }
        for(size_t i=0;i<nav["links"].size();++i) { const auto& item=nav["links"][i];object(item,"navigation.link");SpatialNavLink l;
            l.from=integer(item["from"],"navigation.link.from",1,UINT32_MAX);l.to=integer(item["to"],"navigation.link.to",1,UINT32_MAX);
            if(!ids.count(l.from)||!ids.count(l.to)||l.from==l.to)fail("navigation.link","invalid node reference");
            if(item.hasKey("bidirectional")){if(!item["bidirectional"].isBool())fail("navigation.link.bidirectional","expected boolean");l.bidirectional=item["bidirectional"].getBool();}
            spatialSettings.links.push_back(l);
        }
        if(settings.hasKey("regions")){array(settings["regions"],"spatial.regions");std::set<std::string> names;
            for(size_t i=0;i<settings["regions"].size();++i){const auto& item=settings["regions"][i];ExplorationZone r;r.id=str(item,"id","","region");r.name=str(item,"name",r.id,"region");r.minimum=vector(item["min"],"region.min");r.maximum=vector(item["max"],"region.max");spatial(r.minimum,"region.min");spatial(r.maximum,"region.max");
                if(r.id.empty()||!names.insert(r.id).second||r.minimum.x>=r.maximum.x||r.minimum.y>=r.maximum.y||r.minimum.z>=r.maximum.z)fail("spatial.region","invalid id/bounds");spatialSettings.regions.push_back(r);}}
        if(settings.hasKey("route")){array(settings["route"],"spatial.route");if(settings["route"].size()>256)fail("spatial.route","too many points");for(size_t i=0;i<settings["route"].size();++i){auto p=vector(settings["route"][i],"spatial.route");spatial(p,"spatial.route");spatialSettings.route.push_back(p);}}
    }
    if (gameMode == GameMode::Exploration) {
        const auto& e=d["exploration"]; object(e,"exploration");
        auto region=[&](const J& value,const std::string& path) {
            object(value,path); RegionBounds r;
            r.minimum=vector(value["min"],path+".min"); r.maximum=vector(value["max"],path+".max");
            spatial(r.minimum,path); spatial(r.maximum,path);
            if(r.minimum.x>=r.maximum.x || r.minimum.y>=r.maximum.y || r.minimum.z>=r.maximum.z) fail(path,"min must be below max");
            return r;
        };
        array(e["zones"],"exploration.zones");
        if(e["zones"].size()<1 || e["zones"].size()>16) fail("exploration.zones","requires 1..16 zones");
        std::set<std::string> ids;
        for(size_t i=0;i<e["zones"].size();++i) { const auto& value=e["zones"][i];
            ExplorationZone z; static_cast<RegionBounds&>(z)=region(value,"exploration.zone");
            z.id=str(value,"id","","exploration.zone"); z.name=str(value,"name",z.id,"exploration.zone");
            if(z.id.empty() || !ids.insert(z.id).second) fail("exploration.zone.id","requires unique nonempty id");
            exploration.zones.push_back(z);
        }
        exploration.encounterZone=str(e,"encounterZone","","exploration");
        if(!ids.count(exploration.encounterZone)) fail("exploration.encounterZone","missing zone");
        array(e["spawns"],"exploration.spawns");
        if(e["spawns"].size()<1 || e["spawns"].size()>8) fail("exploration.spawns","requires 1..8 enemies");
        for(size_t i=0;i<e["spawns"].size();++i) { const auto& value=e["spawns"][i];
            ExplorationSpawn spawn; spawn.type=str(value,"type","","exploration.spawn");
            if(spawn.type!="chaser" && spawn.type!="sentry") fail("exploration.spawn.type","unsupported enemy");
            spawn.position=vector(value["position"],"exploration.spawn.position"); spatial(spawn.position,"exploration.spawn.position");
            exploration.spawns.push_back(spawn);
        }
        exploration.observation=region(e["observation"],"exploration.observation");
        exploration.exit=region(e["exit"],"exploration.exit");
        exploration.observerPosition=vector(e["observerPosition"],"exploration.observerPosition");
        exploration.target=vector(e["target"],"exploration.target");
        exploration.landmarkPosition=vector(e["landmarkPosition"],"exploration.landmarkPosition");
        exploration.landmarkScale=vector(e["landmarkScale"],"exploration.landmarkScale");
        spatial(exploration.target,"exploration.target"); spatial(exploration.landmarkPosition,"exploration.landmarkPosition");
        spatial(exploration.landmarkScale,"exploration.landmarkScale");
        if(!exploration.observation.contains(exploration.observerPosition)) fail("exploration.observerPosition","must lie in observation volume");
        if(glm::length(exploration.target-exploration.observerPosition)<3) fail("exploration.target","must be distinct from observation station");
        if(exploration.landmarkScale.x<=0 || exploration.landmarkScale.y<=0 || exploration.landmarkScale.z<=0) fail("exploration.landmarkScale","must be positive");
        exploration.observerEyeHeight=num(e,"observerEyeHeight",1.6,"exploration");
        if(exploration.observerEyeHeight<1 || exploration.observerEyeHeight>2) fail("exploration.observerEyeHeight","must be 1..2");
        exploration.holdSeconds=num(e,"holdSeconds",2.5,"exploration");
        exploration.angleDegrees=num(e,"angleDegrees",6,"exploration");
        exploration.ringRadius=num(e,"ringRadius",2.5,"exploration");
        if(exploration.holdSeconds<.5 || exploration.holdSeconds>10 || exploration.angleDegrees<1 || exploration.angleDegrees>15 || exploration.ringRadius<=0 || exploration.ringRadius>10) fail("exploration","invalid observation tolerance");
        const auto& depths=e["fragmentDepths"]; array(depths,"exploration.fragmentDepths");
        if(depths.size()!=3) fail("exploration.fragmentDepths","requires three depths");
        float previous=0;
        for(size_t i=0;i<3;++i) { float v=number(depths[i],"exploration.fragmentDepths"); if(v<=previous || v>1) fail("exploration.fragmentDepths","must increase within (0,1]"); exploration.fragmentDepths[i]=v; previous=v; }
        array(e["guidePoints"],"exploration.guidePoints");
        if(e["guidePoints"].size()<2 || e["guidePoints"].size()>32) fail("exploration.guidePoints","requires 2..32 points");
        for(size_t i=0;i<e["guidePoints"].size();++i) { auto point=vector(e["guidePoints"][i],"exploration.guidePoints"); spatial(point,"exploration.guidePoints"); exploration.guidePoints.push_back(point); }
    }
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
                else if(s=="ramp" && spatialSettings.enabled)
                    o.shape=ObjectShape::Ramp;
                else
                    fail(p + ".shape", "expected box, plane or spatial ramp");
                o.position = vector(j["position"], p + ".position");
                o.size = vector(j["size"], p + ".size");
                if (o.size.x <= 0 || o.size.z <= 0 ||
                    (o.shape != ObjectShape::Plane ? o.size.y <= 0 : o.size.y != 0))
                    fail(p + ".size",
                         "box requires positive XYZ; XZ plane requires positive X/Z and zero Y");
                if(j.hasKey("rotation")) {if(!spatialSettings.enabled)fail(p+".rotation","requires spatial triangles");o.rotation=vector(j["rotation"],p+".rotation");spatial(o.rotation,p+".rotation");}
                if(j.hasKey("collision")){if(!spatialSettings.enabled || !j["collision"].isBool())fail(p+".collision","requires spatial boolean");o.collision=j["collision"].getBool();}
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
    if(d["geometry"].hasKey("models")) {
        if(!spatialSettings.enabled)fail("geometry.models","requires spatial triangles");const auto& list=d["geometry"]["models"];array(list,"geometry.models");
        for(size_t i=0;i<list.size();++i){const auto& j=list[i];object(j,"model");ModelInstance m;m.source=j;m.id=integer(j["id"],"model.id",0,UINT32_MAX);m.name=str(j,"name","","model");m.asset=str(j,"asset","","model");
            if(m.asset.empty()||!j.hasKey("collisionAsset"))fail("model","asset and explicit collisionAsset required");m.collisionAsset=str(j,"collisionAsset","","model");
            m.position=vec(j,"position",glm::vec3(0),"model");m.rotation=vec(j,"rotation",glm::vec3(0),"model");m.scale=vec(j,"scale",glm::vec3(1),"model");m.material=str(j,"material","default","model");m.color=vec(j,"color",glm::vec3(.8f),"model");
            spatial(m.position,"model.position");spatial(m.rotation,"model.rotation");spatial(m.scale,"model.scale");if(m.scale.x<=0||m.scale.y<=0||m.scale.z<=0)fail("model.scale","positive components required");models.push_back(m);
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
    if (legacyBrushes.size() + objects.size() + models.size() > 4096 || entities.size() > 8192 ||
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
    brushes = legacyBrushes;collisionBrushes=legacyBrushes;
    for (const auto &o : objects) {auto b=compile(o);brushes.push_back(b);if(o.collision)collisionBrushes.push_back(b);}
    for(const auto& m:models) {
        auto compileAsset=[&](const std::string& path) {auto mesh=loadStaticObj((std::filesystem::path(assetBaseDirectory)/path).lexically_normal().string());Brush b;b.id=m.id;b.material=m.material;b.color=m.color;b.vertices=std::move(mesh.vertices);b.faces=std::move(mesh.faces);b.texCoords=std::move(mesh.texCoords);auto transform=staticTransform(m.position,m.rotation,m.scale);
            for(auto& v:b.vertices){v=glm::vec3(transform*glm::vec4(v,1));spatial(v,"model.transformedVertex");}return b;};
        brushes.push_back(compileAsset(m.asset));if(!m.collisionAsset.empty())collisionBrushes.push_back(m.collisionAsset==m.asset?brushes.back():compileAsset(m.collisionAsset));
    }
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
        if (!spatialSettings.enabled && arenaLayout.obstacles.size() > 64)
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
        if(!spatialSettings.enabled)for (const auto &e : entities)
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
    if(gameMode==GameMode::Exploration) {
        if(!arenaLayout.hasFloor || floorBrushId<0 || playerCount!=1 || arenaLayout.obstacles.size()>64) fail("exploration","requires one floor, one player and at most 64 obstacles");
        auto walkable=[&](glm::vec3 p,float radius,const std::string& path) {
            if(std::abs(p.y-arenaLayout.floorY)>.05f || p.x<arenaLayout.minXZ.x+radius || p.x>arenaLayout.maxXZ.x-radius || p.z<arenaLayout.minXZ.y+radius || p.z>arenaLayout.maxXZ.y-radius) fail(path,"must be on and inside floor");
            for(const auto& o:arenaLayout.obstacles) if(p.x>o.min.x-radius && p.x<o.max.x+radius && p.z>o.min.z-radius && p.z<o.max.z+radius) fail(path,"overlaps obstacle");
        };
        auto validRegion=[&](const RegionBounds& r) {
            if(r.minimum.x<arenaLayout.minXZ.x || r.maximum.x>arenaLayout.maxXZ.x || r.minimum.z<arenaLayout.minXZ.y || r.maximum.z>arenaLayout.maxXZ.y || r.minimum.y>arenaLayout.floorY || r.maximum.y<arenaLayout.floorY) fail("exploration.region","must contain floor height and lie inside floor bounds");
        };
        for(const auto& z:exploration.zones) validRegion(z);
        validRegion(exploration.observation); validRegion(exploration.exit);
        if(exploration.observation.minimum.x<=exploration.exit.maximum.x && exploration.observation.maximum.x>=exploration.exit.minimum.x && exploration.observation.minimum.z<=exploration.exit.maximum.z && exploration.observation.maximum.z>=exploration.exit.minimum.z) fail("exploration.exit","must be separate from observation");
        walkable(arenaLayout.playerStart,.34f,"exploration.player");
        walkable(exploration.observerPosition,.34f,"exploration.observerPosition");
        walkable({(exploration.exit.minimum.x+exploration.exit.maximum.x)*.5f,arenaLayout.floorY,(exploration.exit.minimum.z+exploration.exit.maximum.z)*.5f},.34f,"exploration.exit");
        for(const auto& p:exploration.guidePoints) walkable(p,.34f,"exploration.guidePoints");
        for(const auto& spawn:exploration.spawns) {
            walkable(spawn.position,spawn.type=="sentry"?.6f:.34f,"exploration.spawn");
            for(const auto& z:exploration.zones) if(z.id==exploration.encounterZone && !z.contains(spawn.position)) fail("exploration.spawn","must be in encounter zone");
        }
    }
    if(spatialSettings.enabled) {
        spatialWorld=std::make_shared<SpatialWorld>(collisionBrushes,spatialSettings.nodes,spatialSettings.links);
        spatialDiagnostics=spatialWorld->diagnostics();
        for(const auto& entity:entities)if(entity.type==EntityType::PLAYER_START||entity.type==EntityType::RIFT||entity.type==EntityType::SENTRY_SPAWN){
            float radius=entity.type==EntityType::SENTRY_SPAWN?.6f:.34f;
            if(!spatialWorld->isWalkable(entity.position,radius))spatialDiagnostics.push_back("Spawn "+entity.name+" lacks support or clearance");
            bool connected=false;for(const auto& node:spatialSettings.nodes)if(spatialWorld->canTraverse(entity.position,node.position,radius)){connected=true;break;}
            if(!connected)spatialDiagnostics.push_back("Spawn "+entity.name+" has no reachable navigation node");
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
    d["gameMode"] = J(gameMode == GameMode::ArenaSurvival ? "arena_survival" : gameMode == GameMode::Exploration ? "exploration" : "legacy");
    d["environmentPreset"] =
        J(environment.preset == EnvironmentPreset::StarryCrystal ? "starry_crystal" :
          environment.preset == EnvironmentPreset::HollowOrbit ? "hollow_orbit" : "none");
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
    if(!environment.architecture.empty()){env["architecture"]=J(environment.architecture);env["landmarkPosition"]=jvec(environment.landmarkPosition);env["landmarkScale"]=jvec(environment.landmarkScale);}
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
        j["shape"] = J(o.shape == ObjectShape::Box ? "box" : o.shape == ObjectShape::Ramp ? "ramp" : "plane");
        j["position"] = jvec(o.position);
        j["size"] = jvec(o.size);
        if(spatialSettings.enabled){j["rotation"]=jvec(o.rotation);j["collision"]=J(o.collision);}
        appearance(j, o.material, o.texture, o.color, o.surfaceType);
        os.push_back(j);
    }
    g["brushes"] = bs;
    g["objects"] = os;
    if(!models.empty()) {J list=J::array();for(const auto& m:models){J j=original(m.source);j["id"]=J(double(m.id));j["name"]=J(m.name);j["asset"]=J(m.asset);j["collisionAsset"]=J(m.collisionAsset);j["position"]=jvec(m.position);j["rotation"]=jvec(m.rotation);j["scale"]=jvec(m.scale);j["material"]=J(m.material);j["color"]=jvec(m.color);list.push_back(j);}g["models"]=list;}
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
        c.assetBaseDirectory=assetBaseDirectory;
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
        J saved=toJson();
        if(!models.empty())for(size_t i=0;i<models.size();++i)for(const char* key:{"asset","collisionAsset"}) {
            const auto resource=saved["geometry"]["models"][i].getString(key);if(resource.empty())continue;
            auto absolute=std::filesystem::absolute(std::filesystem::path(assetBaseDirectory)/resource);
            saved["geometry"]["models"][i][key]=J(std::filesystem::relative(absolute,std::filesystem::absolute(path).parent_path()).generic_string());
        }
        f << saved.toString() << '\n';
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
