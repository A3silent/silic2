#include "character/character_presentation.h"
#include "engine/shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace silic2 {
namespace {
constexpr float Pi = 3.14159265359f;
const glm::vec3 Warm(1.0f, 0.40f, 0.16f);
const glm::vec3 Hot(1.0f, 0.82f, 0.26f);
const glm::vec3 Cyan(0.25f, 0.92f, 1.0f);
const glm::vec3 Hit(0.80f, 1.0f, 1.0f);
const glm::vec3 Armor(0.28f, 0.23f, 0.42f);
float saturate(float x) { return std::clamp(x, 0.0f, 1.0f); }
glm::mat4 part(const glm::mat4& root, glm::vec3 offset, glm::vec3 size,
               float angle = 0.0f, glm::vec3 axis = glm::vec3(0, 0, 1)) {
    return glm::scale(glm::rotate(glm::translate(root, offset), angle, axis), size);
}
glm::vec3 directionOr(glm::vec3 direction, glm::vec3 fallback) {
    const float magnitude = glm::length(direction);
    return magnitude > 0.0001f ? direction / magnitude : fallback;
}
}

CharacterPresentation::CharacterPresentation() { vertices.reserve(65536); }
CharacterPresentation::~CharacterPresentation() { shutdown(); }

bool CharacterPresentation::init() {
    if (shader) return true;
    try {
        shader = std::make_unique<Shader>("res/shaders/character.vert", "res/shaders/character.frag");
    } catch (...) { return false; }
    GLint linked = GL_FALSE;
    glGetProgramiv(shader->ID, GL_LINK_STATUS, &linked);
    if (!linked) { shader.reset(); return false; }
    GLint previousVao = 0, previousBuffer = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 65536 * sizeof(Vertex), nullptr, GL_STREAM_DRAW);
    const size_t offsets[] = {offsetof(Vertex, position), offsetof(Vertex, normal),
                              offsetof(Vertex, color), offsetof(Vertex, emission)};
    for (unsigned int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, i == 3 ? 1 : 3, GL_FLOAT, GL_FALSE,
                              sizeof(Vertex), reinterpret_cast<void*>(offsets[i]));
    }
    glBindVertexArray(previousVao);
    glBindBuffer(GL_ARRAY_BUFFER, previousBuffer);
    reset();
    return true;
}

void CharacterPresentation::shutdown() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    vao = vbo = 0;
    shader.reset();
}

void CharacterPresentation::reset() {
    time = stepPhase = recoil = muzzle = slideBlend = slideTimer = 0.0f;
    nextShard = 0;
    playerState = FirstPersonView{};
    shards.fill(Shard{});
    vertices.clear();
}

void CharacterPresentation::addShard(const glm::vec3& position, const glm::vec3& velocity,
                                     const glm::vec3& color, float duration, float size) {
    shards[nextShard] = {position, velocity, color, 0.0f, duration, size};
    nextShard = (nextShard + 1) % EffectCapacity;
}

void CharacterPresentation::emit(CharacterEffect effect, const glm::vec3& position,
                                 const glm::vec3& direction) {
    const int count = effect == CharacterEffect::Death ? 18 : 7;
    const glm::vec3 axis = directionOr(direction, glm::vec3(0, 1, 0));
    // Deterministic visual pattern; never consumes the gameplay random stream.
    for (int i = 0; i < count; ++i) {
        const float a = i * 2.39996323f;
        const float y = 1.0f - 2.0f * (i + 0.5f) / count;
        const float radial = std::sqrt(1.0f - y * y);
        const glm::vec3 spread(radial * std::cos(a), y, radial * std::sin(a));
        const bool death = effect == CharacterEffect::Death;
        const bool fire = effect == CharacterEffect::SentryFire;
        addShard(position, spread * (death ? 2.3f : 0.9f) + axis * (fire ? 1.8f : 0.5f),
                 fire ? Hot : (death ? Warm : Hit), death ? 0.55f : 0.16f,
                 death ? 0.10f : 0.045f);
    }
}

void CharacterPresentation::onPrimaryShot() {
    recoil = 1.0f;
    muzzle = 0.035f;
}

void CharacterPresentation::update(float dt, const FirstPersonView& player) {
    if (!std::isfinite(dt) || dt <= 0.0f) return;
    time += dt;
    playerState = player;
    const float speed = std::max(0.0f, player.speed);
    if (player.grounded) stepPhase += speed * dt * 1.8f;
    recoil *= std::exp(-24.0f * dt);
    muzzle = std::max(0.0f, muzzle - dt);
    slideBlend += ((player.sliding ? 1.0f : 0.0f) - slideBlend) * (1.0f - std::exp(-15.0f * dt));
    for (auto& shard : shards) shard.age = std::min(shard.age + dt, shard.duration);
    if (player.sliding && player.grounded && speed > 0.2f) {
        slideTimer += dt;
        if (slideTimer >= 0.04f) {
            slideTimer = std::fmod(slideTimer, 0.04f);
            glm::vec3 forward = directionOr(glm::vec3(player.forward.x, 0, player.forward.z), glm::vec3(0, 0, -1));
            const glm::vec3 side(-forward.z, 0, forward.x);
            for (int sign : {-1, 1}) {
                addShard(player.feet + side * (0.26f * sign) + glm::vec3(0, 0.07f, 0),
                         -forward * 1.8f + side * (0.7f * sign) + glm::vec3(0, 0.35f, 0),
                         Cyan, 0.22f, 0.032f);
            }
        }
    } else { slideTimer = 0.0f; }
}

size_t CharacterPresentation::activeEffectCount() const {
    return static_cast<size_t>(std::count_if(shards.begin(), shards.end(),
        [](const Shard& shard) { return shard.age < shard.duration; }));
}

void CharacterPresentation::triangle(glm::vec3 a, glm::vec3 b, glm::vec3 c,
                                     const glm::vec3& color, float emission) {
    glm::vec3 n = glm::cross(b - a, c - a);
    const float length = glm::length(n);
    if (length < 1e-9f) return;
    n /= length;
    vertices.push_back({a, n, color, emission});
    vertices.push_back({b, n, color, emission});
    vertices.push_back({c, n, color, emission});
}

void CharacterPresentation::crystal(const glm::mat4& transform, const glm::vec3& color, float emission) {
    // Eight-sided bipyramid. Local bounds [-.5,.5], outward CCW winding.
    auto vertex = [&](glm::vec3 p) { return glm::vec3(transform * glm::vec4(p, 1)); };
    const glm::vec3 top = vertex({0, .5f, 0}), bottom = vertex({0, -.5f, 0});
    for (int i = 0; i < 8; ++i) {
        const float a = i * Pi * .25f, b = (i + 1) * Pi * .25f;
        const glm::vec3 p = vertex({.5f * std::cos(a), -.08f, .5f * std::sin(a)});
        const glm::vec3 q = vertex({.5f * std::cos(b), -.08f, .5f * std::sin(b)});
        triangle(top, q, p, color, emission);
        triangle(bottom, p, q, color * .86f, emission);
    }
}

void CharacterPresentation::box(const glm::mat4& transform, const glm::vec3& color, float emission) {
    const glm::vec3 local[] = {{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{-.5f,.5f,-.5f},
                             {-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f}};
    const int faces[][4] = {{0,3,2,1},{4,5,6,7},{0,4,7,3},{1,2,6,5},{3,7,6,2},{0,1,5,4}};
    glm::vec3 v[8];
    for (int i = 0; i < 8; ++i) v[i] = glm::vec3(transform * glm::vec4(local[i], 1));
    for (const auto& f : faces) {
        triangle(v[f[0]], v[f[1]], v[f[2]], color, emission);
        triangle(v[f[0]], v[f[2]], v[f[3]], color, emission);
    }
}

void CharacterPresentation::addCharacter(const CharacterView& c) {
    const float yaw = std::atan2(c.heading.x, c.heading.z);
    glm::mat4 root = glm::rotate(glm::translate(glm::mat4(1), c.position), yaw, glm::vec3(0, 1, 0));
    root = glm::scale(root, glm::max(c.dimensions, glm::vec3(0.01f)));
    const float hit = saturate(c.hitFlash);
    const glm::vec3 shell = glm::mix(Armor, Hit, hit);
    const glm::vec3 core = glm::mix(Warm, Hit, hit);
    if (c.type == EnemyType::Chaser) {
        const float gait = std::sin(time * 10.0f + c.phase) * saturate(c.movement);
        crystal(part(root, {0,.55f,0}, {.85f,.67f,.75f}, gait * .07f), shell, hit);
        crystal(part(root, {0,.84f,.02f}, {.55f,.29f,.64f}), core, .6f);
        box(part(root, {0,.83f,.24f}, {.42f,.044f,.13f}), Hot, 1);
        // Twin talons give a recognizable running silhouette inside the hit volume.
        for (int sign : {-1, 1}) {
            crystal(part(root, {.25f*sign,.18f,.05f}, {.26f,.36f,.30f}, gait * sign * .19f,
                         {1,0,0}), core, .2f);
            crystal(part(root, {.35f*sign,.55f,.13f}, {.22f,.42f,.25f}, sign * -.25f), Warm, .45f);
        }
        crystal(part(root, {0,.53f,.30f}, {.24f,.29f,.16f}), Hot, .8f);
    } else if (c.type == EnemyType::Sentry) {
        // Low four-petal armored turret; tracking/locked direction comes from gameplay.
        crystal(part(root, {0,.28f,0}, {.85f,.56f,.85f}), shell, hit);
        for (int i = 0; i < 4; ++i) {
            const float a = i * Pi * .5f + Pi * .25f;
            crystal(part(root, {.34f*std::cos(a),.5f,.34f*std::sin(a)}, {.25f,.85f,.25f}), shell, hit);
        }
        crystal(part(root, {0,.58f,0}, {.58f,.55f,.58f}), core, .55f);
        box(part(root, {0,.60f,.33f}, {.25f,.20f,.32f}), shell, hit);
        // Warm aperture and expanding four-point crown are exclusively attack cues.
        const float charge = c.charging ? saturate(c.chargeProgress) : 0.0f;
        const glm::vec3 warning = c.aimLocked && c.charging ? Hot : Warm;
        crystal(part(root, {0,.60f,.49f}, {.12f + charge*.17f,.12f + charge*.17f,.06f}), warning, 1);
        if (c.charging) {
            const float span = .17f + .12f * (1.0f - charge);
            for (int i = 0; i < 4; ++i) {
                const float a = Pi * .5f * i;
                const float size = c.aimLocked ? .085f : .05f;
                crystal(part(root, {span*std::cos(a),.60f+span*std::sin(a),.48f},
                             {size,size,.035f}), warning, 1);
            }
        }
    }
}

void CharacterPresentation::renderWorld(const glm::mat4& view, const glm::mat4& projection,
                                        const std::vector<CharacterView>& characters) {
    if (!shader) return;
    vertices.clear();
    for (const auto& character : characters) addCharacter(character);
    for (const auto& shard : shards) {
        if (shard.age >= shard.duration) continue;
        const float t = shard.age;
        const float size = shard.size * (1.0f - t / shard.duration);
        const glm::vec3 position = shard.origin + shard.velocity * t + glm::vec3(0, -2.0f*t*t, 0);
        crystal(part(glm::mat4(1), position, {size,size*1.8f,size}, t*9.0f), shard.color, .9f);
    }
    draw(view, projection, false);
}

void CharacterPresentation::renderFirstPerson(const glm::mat4& projection) {
    if (!shader) return;
    vertices.clear();
    const float movement = playerState.grounded ? saturate(playerState.speed / 8.0f) : 0;
    const float bobX = std::sin(stepPhase) * .006f * movement * (1.0f-slideBlend);
    const float bobY = std::cos(stepPhase*2) * .004f * movement * (1.0f-slideBlend);
    glm::mat4 root = glm::translate(glm::mat4(1), glm::vec3(.18f+bobX, -.15f+bobY-.025f*slideBlend, -.55f+.020f*recoil));
    root = glm::rotate(root, -.10f*slideBlend, glm::vec3(0,0,1));
    root = glm::rotate(root, .035f*recoil, glm::vec3(1,0,0));
    // Camera-local visual offset only; gameplay retains its existing shot origin and aim.
    const glm::vec3 metal(.22f,.25f,.34f), glove(.16f,.20f,.27f);
    box(part(root, {0,-.022f,.11f}, {.083f,.075f,.23f}), metal);
    box(part(root, {0,.005f,.06f}, {.042f,.028f,.16f}), Armor);
    for (int sign : {-1, 1}) {
        crystal(part(root, {.038f*sign,.015f,.09f}, {.019f,.17f,.04f}, Pi*.5f, {1,0,0}), Cyan, .8f);
        box(part(root, {.039f*sign,-.015f,.018f}, {.016f,.042f,.064f}), metal);
    }
    crystal(part(root, {0,-.009f,.025f}, {.037f,.07f,.044f}, Pi*.5f, {1,0,0}), Cyan, .9f);
    box(part(root, {0,-.068f,.14f}, {.048f,.085f,.056f}, -.2f, {1,0,0}), glove);
    // Right grip, left support, and crystal cuff plates. No camera movement is added.
    box(part(root, {.026f,-.077f,.14f}, {.058f,.061f,.085f}), glove);
    box(part(root, {-.07f,-.055f,.075f}, {.075f,.041f,.065f}, -.35f), glove);
    box(part(root, {.057f,-.11f,.21f}, {.067f,.065f,.14f}, -.25f), metal);
    box(part(root, {-.11f,-.087f,.14f}, {.062f,.055f,.15f}, -.55f), metal);
    for (int i = 0; i < 3; ++i)
        box(part(root, {-.073f+i*.019f,-.044f,.044f}, {.012f,.021f,.047f}), {.42f,.48f,.60f});
    crystal(part(root, {.048f,-.074f,.185f}, {.035f,.02f,.07f}), Cyan, .6f);
    if (muzzle > 0) {
        const float size = .035f * muzzle / .035f;
        crystal(part(root, {0,0,-.02f}, {size,size*2.2f,size}, Pi*.5f, {1,0,0}), Hit, 1);
    }
    // Painter order within the small view model keeps its own back faces hidden
    // without clearing or writing into the world depth buffer.
    std::vector<std::array<Vertex, 3>> faces;
    faces.reserve(vertices.size() / 3);
    for (size_t i = 0; i < vertices.size(); i += 3)
        faces.push_back({vertices[i], vertices[i+1], vertices[i+2]});
    std::stable_sort(faces.begin(), faces.end(), [](const auto& a, const auto& b) {
        return a[0].position.z + a[1].position.z + a[2].position.z <
               b[0].position.z + b[1].position.z + b[2].position.z;
    });
    vertices.clear();
    for (const auto& face : faces) vertices.insert(vertices.end(), face.begin(), face.end());
    // A fixed view-model lens prevents sprint/slide FOV from enlarging the gun
    // over the crosshair. World projection and all aiming remain unchanged.
    glm::mat4 weaponProjection = projection;
    const float aspect = projection[1][1] / projection[0][0];
    weaponProjection[1][1] = 1.0f / std::tan(glm::radians(35.0f));
    weaponProjection[0][0] = weaponProjection[1][1] / aspect;
    draw(glm::mat4(1), weaponProjection, true);
}

void CharacterPresentation::draw(const glm::mat4& view, const glm::mat4& projection, bool firstPerson) {
    if (vertices.empty()) return;
    GLint previousProgram, previousVao, previousBuffer, previousDepthFunc;
    GLboolean previousDepthMask;
    const GLboolean depth = glIsEnabled(GL_DEPTH_TEST), blend = glIsEnabled(GL_BLEND);
    const GLboolean cull = glIsEnabled(GL_CULL_FACE);
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);
    glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    if (firstPerson) { glDisable(GL_DEPTH_TEST); glDepthMask(GL_FALSE); }
    else { glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); glDepthMask(GL_TRUE); }
    shader->use();
    shader->setMat4("view", view);
    shader->setMat4("projection", projection);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(previousVao);
    glBindBuffer(GL_ARRAY_BUFFER, previousBuffer);
    glUseProgram(previousProgram);
    glDepthMask(previousDepthMask);
    glDepthFunc(previousDepthFunc);
    if (depth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (cull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
}
} // namespace silic2
