#pragma once

#include <array>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "game/gameplay_types.h"

namespace silic2 {
class Shader;

// Read-only visual snapshot. position is the feet; heading is a world direction.
// Timers/progress are supplied by gameplay, never advanced by the renderer.
struct CharacterView {
    EnemyType type = EnemyType::Chaser;
    glm::vec3 position{0.0f};
    glm::vec3 heading{0.0f, 0.0f, 1.0f};
    glm::vec3 dimensions{0.6f, 1.8f, 0.6f};
    float movement = 0.0f;       // normalized actual movement, not AI intent
    float chargeProgress = 0.0f; // 0..1 across the whole 1 second charge
    bool charging = false;
    bool aimLocked = false;
    float hitFlash = 0.0f;       // 0..1, typically decays over 0.1 seconds
    float phase = 0.0f;          // stable per-enemy animation offset
};

struct FirstPersonView {
    float speed = 0.0f; // horizontal world units/second
    bool grounded = true;
    bool sliding = false;
    glm::vec3 feet{0.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
};

enum class CharacterEffect { Impact, Death, SentryFire };

// Owns only presentation. Call update only for advancing simulation frames;
// rendering never advances time. Reset on every run; shutdown before GL teardown.
class CharacterPresentation {
public:
    CharacterPresentation();
    ~CharacterPresentation();
    CharacterPresentation(const CharacterPresentation&) = delete;
    CharacterPresentation& operator=(const CharacterPresentation&) = delete;

    bool init();
    void shutdown();
    void reset();
    void update(float dt, const FirstPersonView& player);
    void onPrimaryShot(); // only after Weapon::fire returns true
    void emit(CharacterEffect effect, const glm::vec3& position,
              const glm::vec3& direction = glm::vec3(0.0f, 1.0f, 0.0f));
    void renderWorld(const glm::mat4& view, const glm::mat4& projection,
                     const std::vector<CharacterView>& characters);
    // Draw after world transparencies, before post-processing/HUD; uses camera
    // local coordinates with a fixed 70-degree lens and preserves the world depth buffer and all touched GL state.
    void renderFirstPerson(const glm::mat4& projection);
    size_t activeEffectCount() const;
    static constexpr size_t EffectCapacity = 256;

private:
    struct Vertex { glm::vec3 position, normal, color; float emission; };
    struct Shard {
        glm::vec3 origin{0.0f}, velocity{0.0f}, color{0.0f};
        float age = 0.0f, duration = 0.0f, size = 0.0f;
    };
    std::unique_ptr<Shader> shader;
    unsigned int vao = 0, vbo = 0;
    std::vector<Vertex> vertices;
    std::array<Shard, EffectCapacity> shards{};
    size_t nextShard = 0;
    float time = 0.0f, stepPhase = 0.0f, recoil = 0.0f, muzzle = 0.0f;
    float slideBlend = 0.0f, slideTimer = 0.0f;
    FirstPersonView playerState;

    void triangle(glm::vec3 a, glm::vec3 b, glm::vec3 c,
                  const glm::vec3& color, float emission);
    // Tapered pentagonal bone/metal segments; geometry stays in caller's envelope.
    void bone(const glm::mat4& root, glm::vec3 a, glm::vec3 b,
              float radiusA, float radiusB, const glm::vec3& color, float emission = 0.0f);
    void cup(const glm::mat4& root, float opening, const glm::vec3& shell,
             const glm::vec3& lining, float hit);
    friend struct CharacterGeometryProbe; // CPU-only bounds/budget verification.
    void box(const glm::mat4& transform, const glm::vec3& color, float emission = 0.0f);
    void addCharacter(const CharacterView& character);
    void draw(const glm::mat4& view, const glm::mat4& projection, bool firstPerson);
    void addShard(const glm::vec3& position, const glm::vec3& velocity,
                  const glm::vec3& color, float duration, float size);
};
} // namespace silic2
