#include "environment/environment_renderer.h"
#include "engine/shader.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

namespace silic2 {
namespace {
void requireLinked(const Shader& shader) {
    GLint linked = GL_FALSE;
    glGetProgramiv(shader.ID, GL_LINK_STATUS, &linked);
    if (!linked) throw std::runtime_error("Environment shader failed to link");
}
}
EnvironmentRenderer::EnvironmentRenderer(const EnvironmentScene& scene) : scene_(scene) {
    setParticleCount(scene_.settings.particleCount);
    const auto paths=environmentShaderPaths(scene_.settings.preset);
    sky_ = std::make_unique<Shader>("res/shaders/environment_sky.vert", paths.skyFragment);
    crystal_ = std::make_unique<Shader>("res/shaders/environment_crystal.vert", paths.detailFragment);
    particles_ = std::make_unique<Shader>(paths.particleVertex, paths.particleFragment);
    requireLinked(*sky_); requireLinked(*crystal_); requireLinked(*particles_);
    glGenVertexArrays(1, &emptyVao_);
    buildCrystals();
}
EnvironmentRenderer::~EnvironmentRenderer() {
    if (crystalVbo_) glDeleteBuffers(1, &crystalVbo_);
    if (crystalVao_) glDeleteVertexArrays(1, &crystalVao_);
    if (emptyVao_) glDeleteVertexArrays(1, &emptyVao_);
}
void EnvironmentRenderer::reset() { seconds_ = 0; warnings_.fill(-1); observationProgress_=0; observationComplete_=false; }
void EnvironmentRenderer::setObservationState(float progress, bool complete) {
    if (!scene_.observatory) return;
    observationProgress_=std::isfinite(progress)?std::clamp(progress,0.f,1.f):0.f;
    observationComplete_=complete;
}
void EnvironmentRenderer::setTime(float seconds) { seconds_ = std::isfinite(seconds) ? std::max(0.f, seconds) : 0.f; }
void EnvironmentRenderer::setRiftWarnings(const std::array<float, 4>& progress) {
    for (std::size_t i = 0; i < 4; ++i)
        warnings_[i] = std::isfinite(progress[i]) && progress[i] >= 0 ? std::min(progress[i], 1.f) : -1.f;
}
void EnvironmentRenderer::setParticleCount(std::size_t count) { particleCount_ = std::min(count, std::size_t(2000)); }
float EnvironmentRenderer::transformation() const {
    if (scene_.observatory) return observationComplete_ ? 1.f : observationProgress_*.3f;
    return environmentPhase(scene_.settings, seconds_);
}
void EnvironmentRenderer::buildCrystals() {
    const auto data = buildEnvironmentGeometry(scene_);
    crystalVertices_ = static_cast<GLsizei>(data.size()/7);
    glGenVertexArrays(1,&crystalVao_); glGenBuffers(1,&crystalVbo_);
    glBindVertexArray(crystalVao_); glBindBuffer(GL_ARRAY_BUFFER,crystalVbo_);
    glBufferData(GL_ARRAY_BUFFER,data.size()*sizeof(float),data.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,7*sizeof(float),nullptr); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,7*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,7*sizeof(float),(void*)(6*sizeof(float))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}
void EnvironmentRenderer::setObservatoryUniforms(Shader& shader) const {
    shader.setBool("observatory",scene_.observatory);
    shader.setBool("combatWorld",scene_.combatWorld);
    if (!scene_.observatory && !scene_.combatWorld) return;
    shader.setFloat("observationProgress",observationProgress_);
    shader.setBool("calibrated",observationComplete_);
    shader.setVec3("observerEye",scene_.observerEye);
    shader.setVec3("focalPoint",scene_.focalPoint);
    shader.setVec3("landmarkPosition",scene_.landmarkPosition);
    shader.setVec3("landmarkScale",scene_.landmarkScale);
    shader.setVec3("exitCenter",scene_.exitCenter);
    const auto count=std::min(scene_.regions.size(),size_t(8));
    shader.setInt("regionCount",static_cast<int>(count));
    for(size_t i=0;i<count;++i) {
        const auto& r=scene_.regions[i];
        shader.setVec4("regionBounds["+std::to_string(i)+"]",r.minimum.x,r.minimum.y,r.maximum.x,r.maximum.y);
        shader.setInt("regionMaterial["+std::to_string(i)+"]",r.material);
    }
    const auto guides=std::min(scene_.guidePoints.size(),size_t(16));
    shader.setInt("guideCount",static_cast<int>(guides));
    for(size_t i=0;i<guides;++i)shader.setVec3("guidePoints["+std::to_string(i)+"]",scene_.guidePoints[i]);
}
void EnvironmentRenderer::setMapUniforms(Shader& shader) const {
    setObservatoryUniforms(shader);
    shader.setFloat("phase",transformation());
    shader.setFloat("time",seconds_);
    shader.setVec4("riftWarnings",warnings_[0],warnings_[1],warnings_[2],warnings_[3]);
    shader.setVec2("floorCenter",(scene_.minimumXZ+scene_.maximumXZ)*.5f);
    shader.setVec2("floorHalfExtent",(scene_.maximumXZ-scene_.minimumXZ)*.5f);
    for (size_t i = 0; i < scene_.rifts.size(); ++i) {
        shader.setBool("riftActive["+std::to_string(i)+"]",scene_.rifts[i].active);
        shader.setMat4("riftWorldToLocal["+std::to_string(i)+"]",scene_.rifts[i].worldToLocal);
    }
}
void EnvironmentRenderer::renderSky(const glm::mat4& view, const glm::mat4& projection) {
    GLboolean depthWrite; glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);
    const bool depthTest = glIsEnabled(GL_DEPTH_TEST), cull = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glDepthMask(GL_FALSE);
    sky_->use(); sky_->setMat4("inverseProjection",glm::inverse(projection));
    sky_->setMat3("inverseViewRotation",glm::transpose(glm::mat3(view)));
    setObservatoryUniforms(*sky_);
    sky_->setFloat("phase",transformation()); sky_->setFloat("time",seconds_);
    glBindVertexArray(emptyVao_); glDrawArrays(GL_TRIANGLES,0,3); glBindVertexArray(0);
    glDepthMask(depthWrite); if(depthTest) glEnable(GL_DEPTH_TEST); if(cull) glEnable(GL_CULL_FACE);
}
void EnvironmentRenderer::renderDetails(const glm::mat4& view, const glm::mat4& projection) {
    crystal_->use(); crystal_->setMat4("view",view); crystal_->setMat4("projection",projection);
    setObservatoryUniforms(*crystal_);
    crystal_->setVec3("cameraPosition",glm::vec3(glm::inverse(view)[3]));
    crystal_->setFloat("phase",transformation()); crystal_->setFloat("time",seconds_);
    crystal_->setVec4("riftWarnings",warnings_[0],warnings_[1],warnings_[2],warnings_[3]);
    glBindVertexArray(crystalVao_); glDrawArrays(GL_TRIANGLES,0,crystalVertices_);
    GLboolean depthWrite; glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);
    const bool pointSize = glIsEnabled(GL_PROGRAM_POINT_SIZE);
    // Opaque, one-pixel motes stay depth-tested and never veil the combat scene.
    glDepthMask(GL_FALSE); glEnable(GL_PROGRAM_POINT_SIZE);
    particles_->use(); particles_->setMat4("view",view); particles_->setMat4("projection",projection);
    particles_->setFloat("phase",transformation()); particles_->setFloat("time",seconds_);
    particles_->setVec2("floorMinimum",scene_.minimumXZ);
    particles_->setVec2("floorMaximum",scene_.maximumXZ);
    particles_->setFloat("floorY",scene_.floorY);
    glBindVertexArray(emptyVao_); glDrawArrays(GL_POINTS,0,static_cast<GLsizei>(scene_.hasFloor ? particleCount_ : 0));
    glBindVertexArray(0); glDepthMask(depthWrite); if(!pointSize) glDisable(GL_PROGRAM_POINT_SIZE);
}
}
