#include "environment/environment_scene.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace silic2 {

std::vector<float> buildEnvironmentGeometry(const EnvironmentScene& scene) {
    constexpr float pi = 3.14159265359f;
    // Decorative silhouettes follow the authored floor's center and extent.
    // Construction is independent from encounter/weapon random state.
    std::vector<float> data;
    auto triangle = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, float shade) {
        const glm::vec3 n = glm::normalize(glm::cross(b-a, c-a));
        for (auto p : {a,b,c}) data.insert(data.end(), {p.x,p.y,p.z,n.x,n.y,n.z,shade});
    };
    const glm::vec2 centerXZ = (scene.minimumXZ + scene.maximumXZ) * .5f;
    const glm::vec2 halfExtent = (scene.maximumXZ - scene.minimumXZ) * .5f;
    for (int i = 0; scene.hasFloor && i < 76; ++i) {
        const float a = i * 2.f * pi / 76.f;
        // Recover the sample's original ring from its floor half extent plus
        // the preset's setback; translations/resizing need no extra coordinates.
        const float setback = 8.f + 3.f * std::sin(i * 5.17f);
        const float cosA = std::cos(a), sinA = std::sin(a);
        const float ellipse = std::sqrt(1.f / (cosA*cosA/(halfExtent.x*halfExtent.x) +
                                                       sinA*sinA/(halfExtent.y*halfExtent.y)));
        const float radial = ellipse + setback;
        const glm::vec3 base(centerXZ.x + cosA*radial, scene.floorY - 1.f, centerXZ.y + sinA*radial);
        const float h = 5.f + 8.f * (.5f + .5f * std::sin(i*7.31f));
        const float radius = .6f + .8f*(.5f+.5f*std::cos(i*4.71f));
        const glm::vec3 tip = base + glm::vec3(.35f*std::sin(a),h,.35f*std::cos(a));
        for (int j = 0; j < 5; ++j) {
            const float p = j*2.f*pi/5.f, q = (j+1)*2.f*pi/5.f;
            glm::vec3 u = base + glm::vec3(radius*std::cos(p),0,radius*std::sin(p));
            glm::vec3 v = base + glm::vec3(radius*std::cos(q),0,radius*std::sin(q));
            glm::vec3 ut = u + glm::vec3(0,h*.65f,0), vt = v + glm::vec3(0,h*.65f,0);
            triangle(u,ut,vt, float(i%7)/7.f); triangle(u,vt,v,float(i%7)/7.f);
            triangle(ut,tip,vt,float(i%7)/7.f);
        }
    }
    // Open energy apertures, not solid geometry: actors can pass through the
    // hollow rift. Thin angular ribbons frame the spawn without hiding its body.
    for (int i = 0; i < 4; ++i) {
        if (!scene.rifts[i].active) continue;
        const glm::mat4& transform = scene.rifts[i].localToWorld;
        const glm::vec3 center(0,1.25f,0);
        const glm::vec3 right(1,0,0);
        for (int j = 0; j < 8; ++j) {
            const float a = j*2.f*pi/8.f, b = (j+1)*2.f*pi/8.f;
            const glm::vec3 u = right*std::cos(a)*1.05f + glm::vec3(0,std::sin(a)*1.2f,0);
            const glm::vec3 v = right*std::cos(b)*1.05f + glm::vec3(0,std::sin(b)*1.2f,0);
            const glm::vec3 p = glm::vec3(transform * glm::vec4(center+u,1));
            const glm::vec3 q = glm::vec3(transform * glm::vec4(center+v,1));
            const glm::vec3 r = glm::vec3(transform * glm::vec4(center+v*.955f,1));
            const glm::vec3 t = glm::vec3(transform * glm::vec4(center+u*.955f,1));
            const float id = -1.f-i;
            triangle(p,q,r,id); triangle(p,r,t,id);
            triangle(p,r,q,id); triangle(p,t,r,id);
        }
    }
    return data;
}

glm::mat4 environmentRiftTransform(const glm::vec3& position,
                                   const glm::vec3& rotationDegrees,
                                   const glm::vec3& scale) {
    glm::mat4 transform = glm::translate(glm::mat4(1.f),position);
    // Authored XYZ Euler rotation, applied X then Y then Z to local points.
    transform = glm::rotate(transform,glm::radians(rotationDegrees.z),glm::vec3(0,0,1));
    transform = glm::rotate(transform,glm::radians(rotationDegrees.y),glm::vec3(0,1,0));
    transform = glm::rotate(transform,glm::radians(rotationDegrees.x),glm::vec3(1,0,0));
    return glm::scale(transform,scale);
}

float environmentPhase(const EnvironmentSettings& settings, float seconds) {
    if (!std::isfinite(seconds) || settings.preset == EnvironmentPreset::None) return 0.f;
    if (seconds < settings.transformationStart) return 0.f;
    if (settings.transformationDuration <= 0.f) return 1.f;
    const float t = std::clamp((seconds-settings.transformationStart)/settings.transformationDuration,0.f,1.f);
    return t*t*(3.f-2.f*t);
}

EnvironmentScene deriveEnvironmentScene(const Map& map) {
    EnvironmentScene scene;
    scene.settings = map.getEnvironment();
    const auto& layout = map.getArenaLayout();
    scene.hasFloor = layout.hasFloor;
    scene.minimumXZ = layout.minXZ;
    scene.maximumXZ = layout.maxXZ;
    scene.floorY = layout.floorY;
    // Rift slots are semantic identity shared with the director, never a hint
    // from material/name or assumed cardinal world coordinates.
    for (std::size_t i = 0; i < scene.rifts.size(); ++i) {
        auto& rift = scene.rifts[i];
        rift.active = scene.hasFloor && layout.riftActive[i];
        rift.position = layout.rifts[i];
        rift.scale = layout.riftScales[i];
        if (rift.active) {
            rift.localToWorld = environmentRiftTransform(rift.position,layout.riftRotations[i],rift.scale);
            rift.worldToLocal = glm::inverse(rift.localToWorld);
        }
    }
    return scene;
}

} // namespace silic2
