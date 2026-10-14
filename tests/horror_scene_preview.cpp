// Explicit synthetic scene fixture. This uses the production renderers at the
// production 320x200 internal resolution; it is not combat or human playtest data.
#include "character/character_presentation.h"
#include "enemy/enemy_projectile_system.h"
#include "engine/map.h"
#include "engine/map_renderer.h"
#include "engine/pixel_renderer.h"
#include "engine/shader.h"
#include "engine/texture.h"
#include "hud/hud_renderer.h"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stdexcept>

using namespace silic2;
namespace {
struct Shot {
    const char *name;
    float time, charge, hit;
    bool locked, projectile;
};
const Shot shots[] = {
    {"combat-12", 12, 0, 0, false, false},       {"charge-track-44", 44, .45f, 0, false, false},
    {"charge-lock-45", 45, .8f, 0, true, false}, {"shift-47", 47, .8f, 0, true, false},
    {"shift-49", 49, .8f, 0, true, false},       {"hit-49", 49, .8f, 1, true, false},
    {"fire-49", 49, 0, 0, false, true}};
std::vector<unsigned char> pixels(int w, int h) {
    std::vector<unsigned char> data(static_cast<size_t>(w) * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    if (glGetError() != GL_NO_ERROR)
        throw std::runtime_error("fixture readback GL error");
    return data;
}
void save(const std::filesystem::path &path, const std::vector<unsigned char> &data, int w, int h) {
    std::ofstream f(path, std::ios::binary);
    f << "P6\n" << w << ' ' << h << "\n255\n";
    for (int y = h - 1; y >= 0; --y)
        f.write(reinterpret_cast<const char *>(data.data() + static_cast<size_t>(y) * w * 3),
                w * 3);
    if (!f)
        throw std::runtime_error("fixture screenshot write failed");
}
// Matches App's existing octahedron, shader, color and gameplay projectile radius.
class ProjectilePreview {
    Shader shader{"res/shaders/enemy.vert", "res/shaders/enemy.frag"};
    GLuint vao = 0, vbo = 0;

  public:
    ProjectilePreview() {
        const glm::vec3 p[6] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                                {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
        const int faces[] = {2, 4, 0, 2, 1, 4, 2, 5, 1, 2, 0, 5,
                             3, 0, 4, 3, 4, 1, 3, 1, 5, 3, 5, 0};
        std::vector<float> v;
        for (int i : faces) {
            for (int a = 0; a < 3; ++a)
                v.push_back(p[i][a]);
            for (int a = 0; a < 3; ++a)
                v.push_back(p[i][a]);
        }
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                              reinterpret_cast<void *>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }
    ~ProjectilePreview() {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }
    void draw(const glm::mat4 &view, const glm::mat4 &projection,
              const EnemyProjectileSystem &system) {
        shader.use();
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setInt("numLights", 0);
        shader.setVec3("ambientLight", glm::vec3(1));
        shader.setVec3("enemyColor", {1, .3f, .08f});
        glBindVertexArray(vao);
        for (const auto &p : system.projectiles()) {
            shader.setMat4("model", glm::scale(glm::translate(glm::mat4(1), p.position),
                                               glm::vec3(EnemyProjectileSystem::RADIUS)));
            glDrawArrays(GL_TRIANGLES, 0, 24);
        }
        glBindVertexArray(0);
    }
};
void preview(GLFWwindow *window, const std::filesystem::path &output, const std::string& mapPath) {
    Map map;
    if (!map.loadFromFile(mapPath))
        throw std::runtime_error(map.getLastError());
    glfwSetWindowTitle(window,(std::string(map.getDisplayName())+" - SYNTHETIC CAPTURE").c_str());
    MapRenderer environment;
    if (!environment.loadMap(map))
        throw std::runtime_error("fixture map renderer");
    PixelRenderer pixel;
    if (!pixel.init(320, 200))
        throw std::runtime_error("fixture framebuffer");
    CharacterPresentation characters;
    if (!characters.init())
        throw std::runtime_error("fixture characters");
    HudRenderer hud;
    if (!hud.init())
        throw std::runtime_error("fixture HUD");
    ProjectilePreview projectiles;
    const auto &layout = map.getArenaLayout();
    const glm::vec3 camera = layout.playerStart + glm::vec3(0, 1.65f, 0);
    const auto view = glm::lookAt(camera, camera + glm::vec3(0, -.045f, -1), glm::vec3(0, 1, 0));
    const auto projection = glm::perspective(glm::radians(70.f), 320.f / 200.f, .1f, 200.f);
    std::ofstream log(output / "capture.jsonl");
    log << "{\"event\":\"fixture\",\"synthetic_scene\":true,\"human_playtest\":false,\"internal_"
           "width\":320,\"internal_height\":200,\"content_version\":\""
        << map.getContentVersion() << "\"}\n";
    for (auto size : {glm::ivec2(1280, 720), glm::ivec2(800, 600)}) {
        glfwSetWindowSize(window, size.x, size.y);
        glfwPollEvents();
        int w, h, lw, lh;
        glfwGetFramebufferSize(window, &w, &h);
        glfwGetWindowSize(window, &lw, &lh);
        for (const auto &shot : shots) {
            characters.reset();
            FirstPersonView player;
            player.feet = layout.playerStart;
            player.forward = {0, 0, -1};
            characters.update(.125f, player);
            CharacterView chaser;
            chaser.position = layout.playerStart + glm::vec3(-1.25f, 0, -4.8f);
            chaser.heading = {0, 0, 1};
            chaser.movement = .65f;
            chaser.hitFlash = shot.hit;
            chaser.phase = .7f;
            CharacterView sentry;
            sentry.type = EnemyType::Sentry;
            sentry.position = layout.playerStart + glm::vec3(.2f, 0, -5.5f);
            sentry.dimensions = {1.2f, 1, 1.2f};
            sentry.heading = {0, 0, 1};
            sentry.charging = shot.charge > 0;
            sentry.chargeProgress = shot.charge;
            sentry.aimLocked = shot.locked;
            sentry.phase = 1.3f;
            std::vector<CharacterView> actors = {chaser, sentry};
            EnemyProjectileSystem system;
            if (shot.projectile) {
                system.spawn(sentry.position + glm::vec3(-.3f, .9f, 1.5f), {0, 0, 1}, 2);
                characters.emit(CharacterEffect::SentryFire,
                                sentry.position + glm::vec3(0, .6f, .5f), {0, 0, 1});
            }
            HudViewModel vm;
            vm.mapName=map.getDisplayName();
            vm.phase = HudPhase::Playing;
            vm.hp = 72;
            vm.combatSeconds = shot.time;
            vm.livingEnemies = 2;
            vm.playerXZ = {camera.x, camera.z};
            vm.mapMinXZ = layout.minXZ;
            vm.mapMaxXZ = layout.maxXZ;
            vm.hitAge = shot.hit > 0 ? .04f : 1000;
            vm.transformationStart = map.getEnvironment().transformationStart;
            vm.transformationDuration = map.getEnvironment().transformationDuration;
            for (const auto &o : layout.obstacles)
                vm.terrain.push_back({{o.min.x, o.min.z}, {o.max.x, o.max.z}});
            for (const auto &r : layout.rifts)
                vm.rifts.push_back({{r.x, r.z}, true});
            for (const auto &c : actors) {
                HudEnemy e;
                e.worldXZ = {c.position.x, c.position.z};
                e.inView = e.unobstructed = true;
                e.sentry = c.type == EnemyType::Sentry;
                vm.enemies.push_back(e);
            }
            environment.setEnvironmentTime(shot.time);
            environment.setRiftWarnings({.75f, -1, -1, -1});
            auto render = [&] {
                pixel.beginPixelRender();
                glEnable(GL_DEPTH_TEST);
                glDepthMask(GL_TRUE);
                const auto bg = map.getWorldSettings().backgroundColor;
                glClearColor(bg.x, bg.y, bg.z, 1);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                environment.render(view, projection);
                characters.renderWorld(view, projection, actors);
                projectiles.draw(view, projection, system);
                characters.renderFirstPerson(projection);
                pixel.endPixelRender(w, h);
                hud.render(vm, lw, lh, w, h);
                glFinish();
                if (glGetError() != GL_NO_ERROR)
                    throw std::runtime_error("integrated scene GL error");
                return pixels(w, h);
            };
            const auto first = render();
            const auto paused = render();
            if (first != paused)
                throw std::runtime_error("same frozen presentation snapshot changed pixels");
            const auto stem = std::string(shot.name) + "-" + std::to_string(lw);
            save(output / (stem + ".ppm"), first, w, h);
            log << "{\"event\":\"capture\",\"name\":\"" << stem
                << "\",\"combat_seconds\":" << shot.time << ",\"charge_progress\":" << shot.charge
                << ",\"aim_locked\":" << (shot.locked ? "true" : "false")
                << ",\"hit_flash\":" << shot.hit << ",\"projectiles\":" << system.size()
                << ",\"framebuffer_width\":" << w << ",\"framebuffer_height\":" << h
                << ",\"frozen_pixels_equal\":true,\"gl_error\":0}\n";
        }
    }
}
} // namespace
int main(int argc, char **argv) {
    const auto output =
        std::filesystem::path(argc > 1 ? argv[1] : "artifacts/acceptance/horror/presentation");
    std::filesystem::create_directories(output);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto *window =
        glfwCreateWindow(1280, 720, "HOLLOW ORBIT - SYNTHETIC CAPTURE", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 2;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    int result = 0;
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        result = 3;
    else
        try {
            preview(window, output, argc>2?argv[2]:"res/maps/hollow_orbit_arena.json");
            TextureManager::getInstance().clear();
            std::cout << "Synthetic integrated captures passed; not a human playtest\n";
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            result = 4;
        }
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
