#include "engine/collision.h"
#include "engine/map.h"
#include "test_support.h"
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>

namespace {
using namespace silic2;
using silic2::test::require;
using J = SimpleJson;
J read(const std::string &path) {
    std::ifstream f(path);
    std::ostringstream s;
    s << f.rdbuf();
    return J::parse(s.str());
}
void legacyAndMigration() {
    Map map;
    for (const auto *file : {"complex_base", "kailiang_room", "simple_corridor", "test_room",
                             "textured_room", "hollow_orbit_arena", "starry_crystal_arena"}) {
        const std::string path = std::string("res/maps/") + file + ".json";
        require(map.loadFromFile(path), "load existing map " + path + ": " + map.getLastError());
        const auto snapshot = map.toJson().toString();
        const auto count = map.getBrushes().size();
        require(map.loadFromFile(path) && map.getBrushes().size() == count,
                "repeat load replaces scene, never accumulates");
        Map roundtrip;
        require(roundtrip.loadFromJson(J::parse(snapshot),map.getAssetBaseDirectory()), "roundtrip parses");
        require(roundtrip.toJson().toString() == snapshot, "complete map roundtrip stable");
    }
    Map old;
    require(old.loadFromFile("tests/fixtures/starry_crystal_arena_v1.json"),
            "published v1 sample migrates");
    require(old.getGameMode() == GameMode::ArenaSurvival && old.getSchemaVersion() == 2,
            "v1 recognized through content version and anchors");
    const auto &a = old.getArenaLayout();
    require(map.loadFromFile("tests/fixtures/starry_crystal_arena_v2.json"),"published v2 survival fixture");
    const auto &b = map.getArenaLayout();
    require(a.minXZ == b.minXZ && a.maxXZ == b.maxXZ && a.floorY == b.floorY &&
                a.playerStart == b.playerStart && a.rifts == b.rifts &&
                a.sentryPositions == b.sentryPositions,
            "old/new arena has identical authored layout");
    require(a.obstacles.size() == b.obstacles.size(), "migration preserves obstacle count");
    for (size_t i = 0; i < a.obstacles.size(); ++i)
        require(a.obstacles[i].min == b.obstacles[i].min &&
                    a.obstacles[i].max == b.obstacles[i].max,
                "migration preserves collision/nav bounds");
    auto brush = old.getLegacyBrushes()[0];
    brush.material = "ordinary_floor";
    require(old.updateBrush(0, brush) && old.getGameMode() == GameMode::ArenaSurvival,
            "material edit cannot turn off migrated arena");
    auto missing = read("tests/fixtures/starry_crystal_arena_v1.json");
    missing["entities"][1]["name"] = J("missing_rift");
    require(!old.loadFromJson(missing) && old.getLastError().find("anchor") != std::string::npos,
            "incomplete v1 anchors fail explicitly");
}
void dualPresets() {
    Map crystal,horror;
    require(crystal.loadFromFile("tests/fixtures/starry_crystal_arena_v2.json"),"load separate crystal arena");
    require(horror.loadFromFile("res/maps/hollow_orbit_arena.json"),"load preserved horror arena");
    require(crystal.getEnvironmentPreset()==EnvironmentPreset::StarryCrystal && horror.getEnvironmentPreset()==EnvironmentPreset::HollowOrbit,"presets remain distinct");
    require(std::string(crystal.getDisplayName())=="STARRY CRYSTAL" && std::string(horror.getDisplayName())=="HOLLOW ORBIT","map identity follows environment");
    require(horror.getContentVersion()=="hollow-orbit-v1" && crystal.getContentVersion()=="starry-crystal-arena-v2","separate content identity survives restoration");
    require(horror.getArenaLayout().rifts==crystal.getArenaLayout().rifts && horror.getArenaLayout().sentryPositions==crystal.getArenaLayout().sentryPositions,"both maps preserve shared gameplay anchors");
    for(const auto* input:{&crystal,&horror}) {
        Map roundtrip;
        require(roundtrip.loadFromJson(SimpleJson::parse(input->toJson().toString())),"dual preset JSON roundtrip");
        require(roundtrip.getEnvironmentPreset()==input->getEnvironmentPreset(),"roundtrip never downgrades preset");
        auto env=roundtrip.getEnvironment();env.preset=input==&crystal?EnvironmentPreset::HollowOrbit:EnvironmentPreset::StarryCrystal;
        require(roundtrip.setEnvironment(env) && roundtrip.getEnvironmentPreset()==env.preset,"editor can switch either preset");
        require(roundtrip.getGameMode()==GameMode::ArenaSurvival && roundtrip.getArenaLayout().rifts==input->getArenaLayout().rifts,"environment change preserves gameplay/layout");
    }
}
void transactionsAndStrictness() {
    Map m;
    require(m.loadFromFile("tests/fixtures/starry_crystal_arena_v2.json"), "sample");
    const auto original = m.toJson().toString();
    auto rejects = [&](J bad) {
        require(!m.loadFromJson(bad), "invalid document rejected");
        require(!m.getLastError().empty(), "rejection explains field");
        require(m.toJson().toString() == original, "failure preserves whole prior scene");
    };
    J bad = m.toJson();
    bad["schemaVersion"] = J(4.0);
    rejects(bad);
    bad = m.toJson();
    bad["worldSettings"]["gravity"] = J("fast");
    rejects(bad);
    bad = m.toJson();
    bad["entities"][0]["position"] = J::array();
    rejects(bad);
    bad = m.toJson();
    bad["geometry"]["objects"][0]["size"][0] = J(-1.0);
    rejects(bad);
    bad = m.toJson();
    bad["geometry"]["objects"][0]["position"][0] = J(3e38);
    rejects(bad);
    bad = m.toJson();
    bad["geometry"]["objects"][1]["id"] = bad["geometry"]["objects"][0]["id"];
    rejects(bad);
    bad = m.toJson();
    bad["geometry"]["brushes"][0]["faces"][0] = J(999999.0);
    rejects(bad);
    bad = m.toJson();
    bad["entities"][1]["slot"] = J(1.0);
    rejects(bad);
    bad = m.toJson();
    bad["lights"][0]["anchor"]["brushId"] = J(999.0);
    rejects(bad);
    bad = m.toJson();
    bad["lights"][1]["anchor"]["brushId"] = J(6.0);
    rejects(bad);
    bad = m.toJson();
    bad["environment"]["particleCount"] = J(2001.0);
    rejects(bad);
    require(!m.removeEntity(1) && m.toJson().toString() == original,
            "required rift deletion is atomic");
    require(!m.removeBrush(6) && m.toJson().toString() == original,
            "referenced light target deletion is atomic");
    auto o = m.getObjects()[0];
    o.position.x = std::numeric_limits<float>::infinity();
    require(!m.updateObject(0, o) && m.toJson().toString() == original, "nonfinite edit is atomic");
    require(!m.loadFromFile("/tmp/silic2-map-does-not-exist.json") &&
                m.toJson().toString() == original,
            "file error preserves scene");
    Map empty;
    require(empty.validate(), "empty legacy document supports incremental editing");
    GeometryObject plane;
    plane.id = 1;
    plane.shape = ObjectShape::Plane;
    plane.size = {10, 0, 10};
    plane.surfaceType = SurfaceType::FLOOR;
    require(empty.addObject(plane), "new legacy plane");
    require(empty.getArenaLayout().hasFloor, "new plane compiles derived floor");
    require(!empty.addObject(plane) && empty.getObjects().size() == 1,
            "duplicate IDs never silently enter scene");
    Entity e;
    e.name = "start";
    require(empty.addEntity(e), "incremental legacy entity");
    require(!empty.setGameMode(GameMode::ArenaSurvival) && empty.getGameMode() == GameMode::Legacy,
            "incomplete arena switch rejected atomically");
}
void roundtripAndDerived() {
    auto d = read("tests/fixtures/starry_crystal_arena_v2.json");
    d["author"] = J("山 / 🌌");
    d["worldSettings"]["extension"] = J(true);
    d["geometry"]["extension"] = J("retain");
    d["geometry"]["objects"][0]["editorNote"] = J("floor");
    d["geometry"]["brushes"][0]["meshMetadata"] = J(7.0);
    d["entities"][0]["properties"]["futureBehavior"] = J("keep");
    d["entities"][0]["stringProperty"] = J("agent");
    d["entities"][0]["floatProperty1"] = J(12.25);
    d["entities"][0]["intProperty"] = J(8.0);
    d["lights"][0]["editorNote"] = J("light");
    d["lights"][0]["type"] = J("spot");
    d["lights"][0]["innerConeAngle"] = J(17.0);
    d["lights"][0]["outerConeAngle"] = J(39.0);
    Map m;
    require(m.loadFromJson(d), "extended document valid");
    auto mesh = m.getLegacyBrushes()[0];
    for (auto &v : mesh.vertices)
        v.z += 5;
    require(m.updateBrush(0, mesh), "move crystal geometry");
    require(m.getLights()[0].position.z == 5 && m.getSourceLights()[0].position.z == 0,
            "anchored light derived independently from authored fallback");
    require(m.getArenaLayout().obstacles[0].min.z == 3,
            "nav obstacle moves with compiled geometry");
    auto rift = m.getEntities()[1];
    rift.position.x = 3;
    rift.rotation.y = 20;
    rift.scale = {2, 3, .5f};
    require(m.updateEntity(1, rift), "move rotate scale rift");
    require(m.getArenaLayout().rifts[0] == rift.position &&
                m.getArenaLayout().riftScales[0] == rift.scale && m.getLights()[1].position.x == 3,
            "layout and anchor light share moved rift");
    const auto saved = m.toJson();
    require(saved["author"].getString() == "山 / 🌌" &&
                saved["geometry"]["objects"][0]["editorNote"].getString() == "floor" &&
                saved["entities"][0]["properties"]["futureBehavior"].getString() == "keep",
            "extensions survive edits");
    const auto path =
        (std::filesystem::temp_directory_path() / "silic2-map-roundtrip-test.json").string();
    require(m.saveToFile(path), "save complete document");
    Map n;
    require(n.loadFromFile(path), "saved document readable by game");
    std::filesystem::remove(path);
    require(n.toJson().toString() == m.toJson().toString(),
            "save roundtrip preserves all known and extension fields");
    require(n.getEntities()[0].intProperty == 8 && n.getEntities()[0].floatProperty1 == 12.25f &&
                n.getSourceLights()[0].innerConeAngle == 17,
            "typed entity and light fields retained");
    require(n.setGameMode(GameMode::Legacy) &&
                n.getEnvironmentPreset() == EnvironmentPreset::StarryCrystal,
            "gameplay and visual preset independent");
}
} // namespace
void runMapTests() {
    legacyAndMigration();
    dualPresets();
    transactionsAndStrictness();
    roundtripAndDerived();
}
