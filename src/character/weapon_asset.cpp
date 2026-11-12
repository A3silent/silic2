#include "character/weapon_asset.h"
#include "engine/simple_json.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
namespace silic2 {
namespace {
glm::vec3 vector3(const SimpleJson& j, const char* field) {
    const auto& v=j[field];
    if(!v.isArray() || v.size()!=3) throw std::runtime_error(std::string(field)+": expected three numbers");
    glm::vec3 result;
    for(size_t i=0;i<3;++i) {
        if(!v[i].isNumber() || !std::isfinite(v[i].getNumber()) || std::abs(v[i].getNumber())>10000)
            throw std::runtime_error(std::string(field)+": invalid finite coordinate");
        result[i]=float(v[i].getNumber());
    }
    return result;
}
}
glm::mat4 firstPersonWeaponRoot(float phase, float speed, bool grounded, float slide, float recoil,
                              float aimBlend, glm::vec3 sightCenter) {
    const float movement=grounded ? std::clamp(speed/8.0f,0.0f,1.0f) : 0.0f;
    const float x=std::sin(phase)*.006f*movement*(1-slide);
    const float y=std::cos(phase*2)*.004f*movement*(1-slide);
    const float aim=std::clamp(aimBlend,0.0f,1.0f);
    auto offset=glm::mix(glm::vec3(.18f+x,-.162f+y-.025f*slide,-.55f),
                         glm::vec3(-sightCenter.x,-sightCenter.y,-.50f),aim);
    offset.z+=.020f*recoil;
    auto root=glm::translate(glm::mat4(1),offset);
    root=glm::rotate(root,-.10f*slide*(1-aim),glm::vec3(0,0,1));
    // A sighted reticle stays on camera forward, including while sliding or
    // firing. Axial recoil remains visible without lying about the shot ray.
    return glm::rotate(root,.035f*recoil*(1-aim),glm::vec3(1,0,0));
}
bool loadWeaponAsset(const std::string& path, WeaponAsset& out, std::string& status) {
    try {
        std::ifstream stream(path);
        if(!stream) throw std::runtime_error("optional weapon config absent");
        if(std::filesystem::file_size(path)>16384) throw std::runtime_error("weapon config exceeds 16KiB");
        std::string source((std::istreambuf_iterator<char>(stream)),{});
        if(source.size()>16384) throw std::runtime_error("weapon config exceeds 16KiB");
        const auto j=SimpleJson::parse(source);
        if(!j.isObject() || !j["schemaVersion"].isNumber() || j["schemaVersion"].getNumber()!=1)
            throw std::runtime_error("unsupported weapon config schema");
        if(!j["enabled"].isBool()) throw std::runtime_error("enabled must be boolean");
        if(!j["enabled"].getBool()) { status="imported weapon disabled (asset fitting pending)"; return false; }
        if(j.getString("forward")!="-Z" || j.getString("up")!="+Y")
            throw std::runtime_error("weapon must be normalized to forward -Z and up +Y");
        const auto manifest=j.getString("manifest");
        if(manifest.empty()) throw std::runtime_error("missing weapon manifest path");
        WeaponAsset candidate;
        candidate.manifestPath=(std::filesystem::absolute(path).parent_path()/manifest).lexically_normal().string();
        const auto position=vector3(j,"position"), rotation=vector3(j,"rotationDegrees"), scale=vector3(j,"scale");
        for(int i=0;i<3;++i) if(scale[i]<.01f || scale[i]>4 || std::abs(position[i])>2)
            throw std::runtime_error("weapon fit outside safe scale/position limits");
        candidate.fit=staticTransform(position,rotation,scale);
        candidate.muzzle=vector3(j,"muzzle");
        if(j.hasKey("leftHandGrip")) candidate.leftHandGrip=vector3(j,"leftHandGrip");
        if(j.hasKey("rightHandGrip")) candidate.rightHandGrip=vector3(j,"rightHandGrip");
        if(glm::length(candidate.leftHandGrip)>.5f || glm::length(candidate.rightHandGrip)>.5f)
            throw std::runtime_error("hand grip exceeds local fitting envelope");
        candidate.model=loadStaticModelBundle(candidate.manifestPath);
        if(candidate.model.parts.empty() || candidate.model.parts.size()>16)
            throw std::runtime_error("weapon requires 1..16 opaque material parts");
        size_t triangles=0;
        for(const auto& p:candidate.model.parts) {
            triangles+=p.mesh.faces.size()/3;
            if(p.baseColorFactor.a<0.999f) throw std::runtime_error("weapon material must be opaque");
        }
        if(triangles>20000) throw std::runtime_error("weapon exceeds 20000 triangle budget; simplify offline");
        const auto lo=candidate.model.boundsMin, hi=candidate.model.boundsMax;
        for(int i=0;i<3;++i) if(candidate.muzzle[i]<lo[i]-.1f || candidate.muzzle[i]>hi[i]+.1f)
            throw std::runtime_error("muzzle must lie close to model bounds");
        // Bound the rendered envelope before GPU allocation. Actual reticle
        // clearance and hand fitting still require visual acceptance.
        for(int mask=0;mask<8;++mask) {
            const glm::vec3 corner((mask&1)?hi.x:lo.x,(mask&2)?hi.y:lo.y,(mask&4)?hi.z:lo.z);
            const auto v=glm::vec3(candidate.fit*glm::vec4(corner,1));
            if(glm::length(v)>1.2f) throw std::runtime_error("weapon fitting envelope exceeds 1.2 units");
        }
        out=std::move(candidate);status="imported weapon CPU asset ready";return true;
    } catch(const std::exception& e) { status=e.what();return false; }
}
}
