#include "character/character_presentation.h"
#include "engine/shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace silic2 {
namespace {
constexpr float Pi = 3.14159265359f;
const glm::vec3 Warm(0.86f, 0.28f, 0.09f);
const glm::vec3 Hot(1.0f, 0.61f, 0.19f);
const glm::vec3 Dust(0.34f, 0.32f, 0.26f);
const glm::vec3 Hit(0.78f, 0.91f, 0.94f);
const glm::vec3 Bone(0.68f, 0.64f, 0.49f);
const glm::vec3 Tissue(0.19f, 0.23f, 0.18f);
const glm::vec3 Cavity(0.038f, 0.047f, 0.039f);
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
                 fire ? Hot : (death ? Bone : Hit), death ? 0.55f : 0.16f,
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
                         Dust, 0.22f, 0.032f);
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

void CharacterPresentation::bone(const glm::mat4& root, glm::vec3 a, glm::vec3 b,
                                  float radiusA, float radiusB, const glm::vec3& color, float emission) {
    const glm::vec3 axis = directionOr(b-a, glm::vec3(0,1,0));
    const glm::vec3 side = directionOr(glm::cross(axis, std::abs(axis.y) > .9f ?
                                      glm::vec3(1,0,0) : glm::vec3(0,1,0)), glm::vec3(1,0,0));
    const glm::vec3 up = glm::cross(axis, side);
    auto world = [&](glm::vec3 p) { return glm::vec3(root * glm::vec4(p,1)); };
    for (int i = 0; i < 5; ++i) {
        const float t = i * (2*Pi/5), u = (i+1) * (2*Pi/5);
        const glm::vec3 p = side*std::cos(t) + up*std::sin(t);
        const glm::vec3 q = side*std::cos(u) + up*std::sin(u);
        const glm::vec3 ap=world(a+p*radiusA), aq=world(a+q*radiusA);
        const glm::vec3 bp=world(b+p*radiusB), bq=world(b+q*radiusB);
        // Broad uneven mineral/tissue facets, not emissive jewel faces.
        const glm::vec3 tint = color * (i == 1 ? .77f : (i == 3 ? .91f : 1.0f));
        triangle(ap, aq, bq, tint, emission);
        triangle(ap, bq, bp, tint, emission);
        triangle(world(a), aq, ap, tint, emission);
        triangle(world(b), bp, bq, tint, emission);
    }
}

void CharacterPresentation::cup(const glm::mat4& root, float opening,
                                 const glm::vec3& shell, const glm::vec3& lining, float hit) {
    // Large unequal plates and an off-center mouth survive the 320x200 view.
    // The recessed floor/attack origin remains at the actual body center.
    auto world = [&](glm::vec3 p) { return glm::vec3(root * glm::vec4(p,1)); };
    auto ring = [&](int i, int layer) {
        constexpr float radii[12] = {.41f,.44f,.42f,.34f,.29f,.30f,.34f,.36f,.43f,.42f,.31f,.35f};
        constexpr float offsets[12] = {.01f,-.035f,.023f,-.012f,-.04f,.018f,.005f,-.03f,.025f,-.018f,.027f,-.01f};
        const float irregular = offsets[i%12];
        const float angle = (i % 12) * (2*Pi/12) + irregular*1.7f;
        if(layer==0) return glm::vec3(.21f*std::cos(angle),.52f+.21f*std::sin(angle),-.19f);
        if(layer==1) return glm::vec3(radii[i%12]*std::cos(angle),
                                     .51f+radii[i%12]*std::sin(angle),.18f+irregular*2.0f);
        // Mouth is left/up of the shell centroid. Its depth tapers toward the
        // unchanged firing origin, never implying a second damage location.
        return glm::vec3(-.065f+(opening*.88f+irregular*.35f)*std::cos(angle),
                         .58f+(opening+irregular*.35f)*std::sin(angle),.30f+irregular*.5f);
    };
    for (int i = 0; i < 12; ++i) {
        const glm::vec3 a=ring(i,0), b=ring(i+1,0), c=ring(i+1,1), d=ring(i,1);
        const glm::vec3 e=ring(i,2), f=ring(i+1,2);
        const glm::vec3 innerA(a.x*.54f,.52f+(a.y-.52f)*.54f,-.13f);
        const glm::vec3 innerB(b.x*.54f,.52f+(b.y-.52f)*.54f,-.13f);
        const bool softSeam = i==3 || i==7 || i==11;
        const glm::vec3 plate = softSeam ? lining*.8f : shell*(i<3 ? .86f : (i<7 ? .65f : 1.0f));
        const glm::vec3 inner = lining * (i<6 ? .72f : 1.15f);
        triangle(world(a),world(b),world(c),plate*.72f,hit);
        triangle(world(a),world(c),world(d),plate*.72f,hit);
        triangle(world(d),world(c),world(f),plate,hit);
        triangle(world(d),world(f),world(e),plate,hit);
        triangle(world(e),world(f),world(innerB),inner,hit*.25f);
        triangle(world(e),world(innerB),world(innerA),inner,hit*.25f);
        // Unequal sutures, rather than a repeating spoke on every segment.
        bone(root, d, e, softSeam?.018f:(i<3?.031f:.016f), .012f, plate, hit);
    }
    bone(root,{-.18f,.37f,.23f},{-.12f,.13f,.12f},.046f,.022f,lining,hit*.25f);
    bone(root,{.17f,.29f,.10f},{.24f,.075f,.06f},.065f,.026f,lining*.75f,hit*.25f);
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
    const glm::vec3 shell = glm::mix(Bone, Hit, hit);
    const glm::vec3 tendon = glm::mix(Tissue, Hit*.6f, hit);
    if (c.type == EnemyType::Chaser) {
        const float gait = std::sin(time * 10.0f + c.phase) * saturate(c.movement);
        const float contraction = .012f * gait;
        // A dark continuous inner body keeps the real hittable center legible.
        // The cage has actual gaps, an uneven spine and a long facial cleft.
        bone(root, {-.035f,.28f,-.12f}, {.045f,.81f,-.10f}, .13f,.105f,tendon,hit*.35f);
        bone(root, {-.075f,.27f,-.22f}, {-.035f,.83f,-.19f}, .045f,.033f,shell,hit);
        for (int rib = 0; rib < 4; ++rib) {
            const float y = .40f + rib*.105f;
            const float radius = .265f + (rib==2 ? .015f : 0) + contraction;
            // Two offset rib arcs terminate beside a vertical front opening.
            for (int sign : {-1,1}) {
                for (int segment=0; segment<4; ++segment) {
                    const float a = -.5f*Pi + segment*.68f;
                    const float b = -.5f*Pi + (segment+1)*.68f;
                    glm::vec3 p(sign*radius*std::cos(a),y+sign*.018f,.25f*std::sin(a));
                    glm::vec3 q(sign*radius*std::cos(b),y+sign*.018f+.014f,.25f*std::sin(b));
                    bone(root,p,q,.034f,.025f,shell,hit);
                }
            }
        }
        // Split, eyeless face: two unequal ossified plates framing a deep slit.
        bone(root,{-.115f,.77f,.08f},{-.070f,.965f,.02f},.090f,.040f,shell,hit);
        bone(root,{.115f,.79f,.06f},{.095f,.91f,.11f},.065f,.050f,shell*.83f,hit);
        bone(root,{.003f,.79f,.17f},{.014f,.94f,.09f},.015f,.01f,Cavity);
        // Folded forelimbs and reversed shins remain entirely inside the body.
        for(int sign : {-1,1}) {
            const float phase = gait*sign*.023f;
            const glm::vec3 shoulder(.30f*sign,.72f,0);
            const glm::vec3 elbow(.38f*sign,.39f,-.08f+phase);
            const glm::vec3 wrist(.23f*sign,.52f,.27f);
            bone(root,shoulder,elbow,.043f,.029f,shell,hit);
            bone(root,elbow,wrist,.028f,.013f,shell*.8f,hit);
            const glm::vec3 hip(.12f*sign,.35f,-.05f);
            const glm::vec3 knee(.23f*sign,.19f,.18f+phase);
            const glm::vec3 foot(.28f*sign,.032f,-.20f+phase);
            bone(root,hip,knee,.061f,.039f,tendon,hit*.5f);
            bone(root,knee,foot,.034f,.018f,shell,hit);
        }
    } else if (c.type == EnemyType::Sentry) {
        const float charge = c.charging ? saturate(c.chargeProgress) : 0.0f;
        const float opening = .18f + .12f*charge;
        cup(root, opening, shell, glm::mix(glm::vec3(.34f,.255f,.225f),Hit*.6f,hit), hit);
        // Rooted calcareous organ, with three irregular supporting folds.
        for(int i=0;i<3;++i) {
            const float a=i*(2*Pi/3)+.4f;
            const glm::vec3 joint(.35f*std::cos(a),.19f,.28f*std::sin(a));
            bone(root,{.1f*std::cos(a),.40f,-.12f},joint,.105f,.064f,tendon,hit*.4f);
            bone(root,joint,{joint.x*.93f,.045f,joint.z},.066f,.028f,shell,hit);
        }
        // Deep opaque floor supports the existing solid collision volume.
        bone(root,{0,.52f,-.17f},{0,.52f,-.11f},.125f,.12f,Cavity);
        if(c.charging) {
            const glm::vec3 warning = c.aimLocked ? Hot : Warm;
            const float pupil = c.aimLocked ? .10f : .035f+.04f*charge;
            bone(root,{0,.52f,-.10f},{0,.52f,-.055f},pupil,pupil,warning,1);
            // Unequal warm creases inside the actual eccentric mouth remain
            // separate from cold hit flashes; the attack pupil stays centered.
            for(int i=0;i<3;++i) {
                const float a=.65f+i*2.0f;
                const glm::vec3 lip(-.065f+opening*.83f*std::cos(a),.58f+opening*.95f*std::sin(a),.31f);
                const float length = i==1 ? .065f : .035f;
                const glm::vec3 tip=lip+glm::vec3(-length*std::cos(a),-length*std::sin(a),0);
                bone(root,lip,tip,c.aimLocked?.023f:.012f,.008f,warning,1);
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
        // Flat dry flakes and dirt; no glowing diamond silhouettes.
        box(part(glm::mat4(1), position, {size,size*.23f,size*.72f}, t*9.0f),
            shard.color, shard.duration < .2f ? .65f : 0.0f);
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
    const glm::vec3 metal(.24f,.26f,.24f), glove(.15f,.145f,.12f);
    const glm::vec3 worn(.46f,.44f,.36f), rust(.28f,.20f,.14f);
    // Worn receiver, separate tubular barrel, iron sight and a recessed bore.
    box(part(root, {0,-.022f,.11f}, {.083f,.075f,.23f}), metal);
    bone(root,{0,-.010f,.10f},{0,-.010f,-.055f},.029f,.024f,metal);
    bone(root,{0,-.010f,-.056f},{0,-.010f,-.059f},.018f,.018f,Cavity);
    for(int i=0;i<3;++i) {
        box(part(root,{0,.021f,.02f+i*.043f},{.074f,.011f,.016f}),worn);
        box(part(root,{.042f,-.015f,.04f+i*.045f},{.003f,.018f,.017f}),rust);
    }
    box(part(root,{0,.031f,-.015f},{.008f,.029f,.012f}),worn);
    box(part(root,{0,.013f,.165f},{.048f,.009f,.022f}),worn);
    // Restrained bone splint wired to the left side, not a luminous power gem.
    bone(root,{-.044f,-.017f,.035f},{-.046f,-.010f,.145f},.010f,.007f,Bone*.7f);
    for(int i=0;i<2;++i)
        box(part(root,{-.044f,-.010f,.06f+i*.05f},{.021f,.035f,.007f}),metal*.65f);
    box(part(root,{.043f,-.006f,.113f},{.004f,.012f,.014f}),{.33f,.37f,.22f});
    box(part(root, {0,-.068f,.14f}, {.048f,.085f,.056f}, -.2f, {1,0,0}), glove);
    box(part(root, {.026f,-.077f,.14f}, {.058f,.061f,.085f}), glove);
    box(part(root, {-.07f,-.055f,.075f}, {.075f,.041f,.065f}, -.35f), glove);
    box(part(root, {.057f,-.11f,.21f}, {.067f,.065f,.14f}, -.25f), metal);
    box(part(root, {-.11f,-.087f,.14f}, {.062f,.055f,.15f}, -.55f), metal);
    for (int i = 0; i < 3; ++i)
        box(part(root, {-.073f+i*.019f,-.044f,.044f}, {.012f,.021f,.047f}), worn*.75f);
    // Abraded receiver edges are sparse high-value marks at 320x200.
    box(part(root,{.039f,.013f,.135f},{.005f,.006f,.07f}),worn);
    box(part(root,{-.028f,.018f,.196f},{.025f,.004f,.012f}),worn);
    if (muzzle > 0) {
        const float size = .035f * muzzle / .035f;
        bone(root,{0,-.01f,-.06f},{0,-.01f,-.11f},size,0,Hit,1);
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
