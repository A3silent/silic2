#pragma once

#include <array>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace silic2 {
struct CharacterView;
class Shader;

// Each species is one immutable vertex stream. One palette of rigid transforms
// articulates all its pieces in a single draw per animal.
class CrystalCreatures {
public:
    CrystalCreatures();
    ~CrystalCreatures();
    bool init();
    void draw(const glm::mat4& view, const glm::mat4& projection,
              const std::vector<CharacterView>& characters, float time);
private:
    struct Range { int first = 0, count = 0; };
    struct MinionInstance {
        glm::vec4 feetAndSinYaw, pose;
        float hit;
    };
    std::array<Range, 5> ranges{};
    std::array<std::vector<MinionInstance>, 2> minions;
    std::unique_ptr<Shader> shader;
    std::unique_ptr<Shader> minionShader;
    unsigned int vao = 0, buffer = 0, instanceBuffer = 0;
    int minionProjection = -1, minionSpecies = -1;
    int projectionLocation = -1, poseLocation = -1, rootLocation = -1;
    int attackLocation = -1, hitLocation = -1, marksLocation = -1;
};
}
