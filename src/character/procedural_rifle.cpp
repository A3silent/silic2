#include "character/procedural_rifle.h"
#include "engine/simple_json.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>
namespace silic2 {
namespace {
constexpr float Pi=3.14159265359f;
enum Material : size_t { Armor, Facet, Edge, Cavity, RedReflect, Energy, Core, Optic, Reticle, Count };
const std::array<glm::vec3,Count> Palette={
    glm::vec3(.065f,.060f,.078f),glm::vec3(.17f,.163f,.20f),glm::vec3(.34f,.33f,.39f),
    glm::vec3(.014f,.012f,.019f),glm::vec3(.23f,.018f,.035f),
    glm::vec3(1,.008f,.025f),glm::vec3(1,.42f,.43f),glm::vec3(.11f,.10f,.14f),glm::vec3(1,.035f,.012f)};
struct Builder {
    std::array<StaticModelPart,Count> parts;
    Builder(){for(size_t i=0;i<Count;++i)parts[i].baseColorFactor=glm::vec4(Palette[i],1);}
    void tri(glm::vec3 a,glm::vec3 b,glm::vec3 c,size_t material) {
        auto normal=glm::cross(b-a,c-a);if(glm::length(normal)<1e-9f)return;
        normal=glm::normalize(normal);auto& m=parts[material].mesh;
        for(const auto p:{a,b,c}) {m.faces.push_back(uint32_t(m.vertices.size()));m.vertices.push_back(p);m.normals.push_back(normal);m.texCoords.emplace_back(0);}
    }
    void quad(glm::vec3 a,glm::vec3 b,glm::vec3 c,glm::vec3 d,size_t material){tri(a,b,c,material);tri(a,c,d,material);}
    // Faceted prism from a z/y silhouette. Raised off-center ridges break broad
    // armor into angular planes; side profiles are authored rather than boxes.
    void plate(const std::vector<glm::vec2>& profile,float halfWidth,float ridge=.006f,size_t material=Armor,float capBevel=0) {
        glm::vec2 center(0);for(auto p:profile)center+=p;center/=float(profile.size());
        for(int sign:{-1,1}) {
            const glm::vec3 apex(sign*(halfWidth+ridge),center.y+.003f,center.x-.008f);
            for(size_t i=0;i<profile.size();++i) {
                const auto a=profile[i],b=profile[(i+1)%profile.size()];
                const size_t face=material==Armor?(i%4==0?Facet:Armor):material;
                tri(apex,{sign*halfWidth,a.y,a.x},{sign*halfWidth,b.y,b.x},face);
            }
        }
        for(size_t i=0;i<profile.size();++i) {
            const auto a=profile[i],b=profile[(i+1)%profile.size()];
            const bool top=(a.y+b.y)*.5f>center.y;
            if(capBevel>0) {
                auto outA=glm::normalize(a-center)*capBevel,outB=glm::normalize(b-center)*capBevel;
                const float edge=std::min(.003f,halfWidth*.15f);
                const glm::vec3 la(-halfWidth,a.y,a.x),lb(-halfWidth,b.y,b.x),
                    li(-halfWidth+edge,a.y,a.x),lj(-halfWidth+edge,b.y,b.x),
                    ma(0,a.y+outA.y,a.x+outA.x),mb(0,b.y+outB.y,b.x+outB.x),
                    ri(halfWidth-edge,a.y,a.x),rj(halfWidth-edge,b.y,b.x),
                    ra(halfWidth,a.y,a.x),rb(halfWidth,b.y,b.x);
                quad(la,lb,lj,li,Edge);quad(li,lj,mb,ma,top?Facet:Armor);
                quad(ma,mb,rj,ri,top?Armor:Facet);quad(ri,rj,rb,ra,Edge);
            } else quad({-halfWidth,a.y,a.x},{halfWidth,a.y,a.x},{halfWidth,b.y,b.x},{-halfWidth,b.y,b.x},top?Facet:material);
        }
    }
    // A tapered armor shard with a genuine thickness/gap from the underlying spine.
    void shard(float sign,const std::vector<glm::vec2>& profile,float innerX,float outerX) {
        glm::vec2 center(0);for(auto p:profile)center+=p;center/=float(profile.size());
        const glm::vec3 apex(sign*outerX,center.y+.005f,center.x-.006f);
        for(size_t i=0;i<profile.size();++i) {
            const auto a=profile[i],b=profile[(i+1)%profile.size()];
            tri(apex,{sign*innerX,a.y,a.x},{sign*innerX,b.y,b.x},i%3==0?Facet:Armor);
            quad({sign*innerX,a.y,a.x},{sign*innerX,b.y,b.x},{sign*(innerX-.006f),b.y,b.x},{sign*(innerX-.006f),a.y,a.x},Armor);
        }
    }
    // Shared miter vertices keep the energy strip watertight at sharp bends.
    // Per-point normals allow a single strip to turn from a dorsal surface over
    // the rear bevel without a disconnected top ribbon / back-cap ribbon.
    void foldedRibbon(const std::vector<glm::vec3>& points,float width,
                      const std::vector<glm::vec3>& normals,size_t material) {
        if(points.size()<2)return;
        std::vector<glm::vec3> left(points.size()),right(points.size());
        for(size_t i=0;i<points.size();++i) {
            const auto previous=points[i]-points[i?i-1:0];
            const auto next=points[std::min(i+1,points.size()-1)]-points[i];
            const auto side=[&](glm::vec3 along) {
                auto v=glm::cross(along,normals[i]);
                return glm::length(v)>1e-7f?glm::normalize(v):glm::vec3(1,0,0);
            };
            auto a=side(i?previous:next),b=side(i+1<points.size()?next:previous);
            if(glm::dot(a,b)<0)a=-a;
            auto miter=glm::normalize(a+b);
            const auto offset=miter*(width*.5f/std::max(.45f,glm::dot(miter,b)));
            left[i]=points[i]-offset;right[i]=points[i]+offset;
        }
        for(size_t i=1;i<points.size();++i)quad(left[i-1],left[i],right[i],right[i-1],material);
    }
    void ribbon(const std::vector<glm::vec3>& points,float width,glm::vec3 normal,size_t material) {
        foldedRibbon(points,width,std::vector<glm::vec3>(points.size(),normal),material);
    }
    void beam(glm::vec3 a,glm::vec3 b,float width,float depth,size_t material) {
        const auto axis=glm::normalize(b-a);
        const auto across=glm::normalize(glm::cross(axis,glm::vec3(0,0,1)))*width*.5f;
        const glm::vec3 dz(0,0,depth*.5f);
        quad(a-across+dz,b-across+dz,b+across+dz,a+across+dz,material);
        quad(a-across-dz,a+across-dz,b+across-dz,b-across-dz,material);
        quad(a+across-dz,a+across+dz,b+across+dz,b+across-dz,Facet);
        quad(a-across-dz,b-across-dz,b-across+dz,a-across+dz,material);
        quad(a-across-dz,a-across+dz,a+across+dz,a+across-dz,material);
        quad(b-across-dz,b+across-dz,b+across+dz,b-across+dz,material);
    }
    WeaponAsset finish() {
        WeaponAsset asset;asset.proceduralMaterial=true;asset.localBloom=true;
        asset.muzzle={0,-.006f,-.451f};asset.sightCenter={0,.095f,.08f};
        asset.model.boundsMin=glm::vec3(std::numeric_limits<float>::max());asset.model.boundsMax=-asset.model.boundsMin;
        for(size_t i=0;i<Count;++i)if(!parts[i].mesh.vertices.empty()) {
            const size_t index=asset.model.parts.size();
            if(i==Energy||i==Core)asset.emissiveParts.push_back(index);
            if(i==Reticle)asset.reticlePart=index;
            for(const auto p:parts[i].mesh.vertices){asset.model.boundsMin=glm::min(asset.model.boundsMin,p);asset.model.boundsMax=glm::max(asset.model.boundsMax,p);}
            asset.model.parts.push_back(std::move(parts[i]));
        }
        return asset;
    }
};
void sight(Builder& b) {
    // Open \_/ yoke. There is deliberately NO top beam, roof, glass or ring.
    std::array<size_t,Count> first;for(size_t i=0;i<Count;++i)first[i]=b.parts[i].mesh.vertices.size();
    const float z=.08f, bottom=.066f;
    const std::array<glm::vec3,4> rim={glm::vec3(-.049f,.130f,z),glm::vec3(-.031f,bottom,z),glm::vec3(.031f,bottom,z),glm::vec3(.049f,.130f,z)};
    for(size_t i=1;i<rim.size();++i)b.beam(rim[i-1],rim[i],.011f,.033f,Optic);
    b.beam({-.027f,.054f,z},{.027f,.054f,z},.014f,.053f,Armor);
    b.beam({0,.027f,z},{0,.052f,z},.032f,.035f,Optic);
    // Rear-facing red inset; narrow, with a low-bloom region around the dot.
    std::vector<glm::vec3> red;
    for(const auto p:rim)red.push_back(p+glm::vec3(0,0,.0172f));
    b.ribbon(red,.0034f,{0,0,1},Energy);
    b.ribbon(red,.0010f,{0,0,1},Core);
    // Tiny split bevels at the two tips preserve the pointed silhouette.
    for(int sign:{-1,1})b.tri({sign*.042f,.130f,z+.018f},{sign*.051f,.140f,z+.005f},{sign*.056f,.126f,z+.010f},Edge);
    // Keep the established eyeline and complete gun pose. Only the original
    // optic assembly is scaled, with a narrow mounting adapter added below it.
    const glm::vec3 pivot(0,.095f,.08f);
    for(size_t i=0;i<Count;++i)for(size_t v=first[i];v<b.parts[i].mesh.vertices.size();++v)
        b.parts[i].mesh.vertices[v]=pivot+(b.parts[i].mesh.vertices[v]-pivot)*.6f;
    b.beam({0,.027f,.08f},{0,.0552f,.08f},.0192f,.021f,Optic);
    // Widen the complete reduced housing and mounting support around the same
    // eyeline, preserving height/depth. Inverse-transpose keeps facets lit
    // correctly under this nonuniform scale; the reticle below stays circular.
    constexpr float widthScale=1.35f;
    for(size_t i=0;i<Count;++i)for(size_t v=first[i];v<b.parts[i].mesh.vertices.size();++v) {
        auto& mesh=b.parts[i].mesh;
        mesh.vertices[v].x=pivot.x+(mesh.vertices[v].x-pivot.x)*widthScale;
        auto& normal=mesh.normals[v];
        normal=glm::normalize(glm::vec3(normal.x/widthScale,normal.y,normal.z));
    }
    // Preserve the small readable disk radius instead of shrinking it to one
    // unstable pixel. The center follows the scaled optic's depth, x/y stay 0.
    const glm::vec3 center=pivot+(glm::vec3(0,.095f,.114f)-pivot)*.6f;
    for(int i=0;i<12;++i) {
        const float a=2*Pi*i/12,c=2*Pi*(i+1)/12;
        b.tri(center,center+glm::vec3(std::cos(a),std::sin(a),0)*.0021f,
              center+glm::vec3(std::cos(c),std::sin(c),0)*.0021f,Reticle);
    }
}
void appendCompatibilitySight(WeaponAsset& asset) {
    Builder b;sight(b);auto optic=b.finish();
    const auto inverse=glm::inverse(asset.fit);const auto normal=glm::transpose(glm::mat3(asset.fit));
    const size_t offset=asset.model.parts.size();
    for(auto& part:optic.model.parts) {
        for(auto& p:part.mesh.vertices)p=glm::vec3(inverse*glm::vec4(p,1));
        for(auto& n:part.mesh.normals)n=glm::normalize(normal*n);
        asset.model.parts.push_back(std::move(part));
    }
    asset.reticlePart=offset+optic.reticlePart;
    for(auto index:optic.emissiveParts)asset.emissiveParts.push_back(offset+index);
    asset.localBloom=true;
}
}
WeaponAsset buildProceduralRifle() {
    Builder b;
    // Narrow internal chassis: it never fills the gaps between the outer gills.
    b.plate({{-.425f,.004f},{-.365f,.026f},{-.10f,.030f},{.14f,.030f},{.165f,-.022f},{.02f,-.045f},{-.35f,-.034f}},.025f,.004f);
    // Six separated swept gill/shutter shards on BOTH sides. Red chambers remain
    // visible between their diagonals; each shard tapers into a sharp front tooth.
    for(int i=0;i<6;++i) {
        const float z=-.350f+i*.046f;
        for(float sign:{-1.f,1.f}) {
            const float x=sign*.037f;
            const std::vector<glm::vec3> channel={{x,-.027f,z-.015f},{x,.005f,z+.005f},{x,.026f,z+.027f}};
            b.ribbon(channel,.021f,{sign,0,0},Energy);
            std::vector<glm::vec3> core=channel;for(auto& p:core)p.x+=sign*.0003f;
            b.ribbon(core,.0045f,{sign,0,0},Core);
            b.shard(sign,{{z-.039f,.038f},{z+.017f,.030f},{z+.009f,.007f},{z-.020f,-.045f},{z-.045f,-.043f}},.039f,.055f);
        }
    }
    // Long angular upper shroud and lower keel: split in the silhouette, with
    // alternating spear-like fins rather than a cylindrical handguard.
    b.plate({{-.423f,.014f},{-.286f,.058f},{-.203f,.052f},{-.325f,.030f},{-.387f,-.003f}},.027f,.004f);
    b.plate({{-.397f,-.036f},{-.280f,-.055f},{-.067f,-.041f},{-.125f,-.022f},{-.330f,-.030f}},.030f,.004f);
    // Interlocking receiver plates have an actual broken, angular outline.
    b.plate({{-.112f,.043f},{-.014f,.061f},{.050f,.047f},{.108f,.032f},{.046f,-.005f},{-.012f,.009f}},.043f,.011f,Armor,.005f);
    b.plate({{-.090f,-.005f},{-.010f,-.066f},{.052f,-.028f},{.138f,-.019f},{.074f,.013f},{.006f,-.006f}},.043f,.009f,Armor,.004f);
    b.plate({{.047f,.052f},{.134f,.046f},{.188f,.012f},{.125f,-.011f},{.101f,.013f}},.036f,.008f,Armor,.004f);
    // Main receiver-to-stock channel is emitted after all armor below.
    // Zigzag dorsal channel is visible from hip and rear three-quarter views.
    const std::vector<glm::vec3> dorsal={{-.014f,.059f,-.274f},{.007f,.058f,-.210f},{-.010f,.054f,-.169f},{.018f,.050f,-.112f},{0,.062f,-.013f},{.016f,.053f,.043f},{0,.050f,.125f}};
    b.ribbon(dorsal,.007f,{0,1,0},RedReflect);
    auto top=dorsal;for(auto& p:top)p.y+=.0005f;b.ribbon(top,.0052f,{0,1,0},Energy);
    for(auto& p:top)p.y+=.0003f;b.ribbon(top,.0025f,{0,1,0},Core);
    // Curved forward-swept detachable magazine, four broad bends, split side armor.
    b.plate({{-.052f,-.049f},{.009f,-.048f},{-.004f,-.112f},{-.035f,-.170f},{-.082f,-.219f},{-.125f,-.195f},{-.081f,-.133f}},.027f,.008f);
    for(float sign:{-1.f,1.f}) {
        const float x=sign*.036f;
        std::vector<glm::vec3> line={{x,-.055f,-.008f},{x,-.110f,-.023f},{x,-.143f,-.053f},{x,-.170f,-.067f},{x,-.202f,-.103f}};
        b.ribbon(line,.009f,{sign,0,0},RedReflect);
        for(auto& p:line)p.x+=sign*.0004f;b.ribbon(line,.0054f,{sign,0,0},Energy);
        for(auto& p:line)p.x+=sign*.0002f;b.ribbon(line,.0025f,{sign,0,0},Core);
    }
    // An angled grip and open trigger guard are gun parts, never a hand mesh.
    b.plate({{.084f,-.035f},{.126f,-.032f},{.158f,-.136f},{.124f,-.155f},{.102f,-.090f}},.019f,.005f);
    b.beam({0,-.044f,.028f},{0,-.092f,.046f},.008f,.008f,Armor);
    b.beam({0,-.092f,.046f},{0,-.095f,.111f},.009f,.009f,Armor);
    b.beam({0,-.095f,.111f},{0,-.051f,.121f},.009f,.009f,Armor);
    b.beam({0,-.044f,.065f},{0,-.074f,.059f},.005f,.008f,Energy);
    // Tapered shoulder stock split into a blade spine, angular butt and negative
    // lower cutout. The sharp rear edge is honest geometry, not a box texture.
    b.plate({{.133f,.035f},{.237f,.048f},{.322f,.031f},{.337f,.011f},{.271f,-.014f},{.198f,-.006f}},.030f,.008f,Armor,.005f);
    b.plate({{.290f,.018f},{.337f,.011f},{.322f,-.116f},{.299f,-.104f},{.276f,-.041f},{.247f,-.016f}},.032f,.007f,Armor,.005f);
    // Stepped dorsal carapace overlays and a beveled rear cap break the large
    // camera-facing stock into real left/right planes, not a flat purple slab.
    b.plate({{.135f,.052f},{.215f,.068f},{.260f,.058f},{.228f,.044f},{.168f,.039f}},.026f,.004f,Armor,.004f);
    b.plate({{.225f,.061f},{.296f,.049f},{.329f,.031f},{.281f,.031f},{.252f,.041f}},.025f,.004f,Armor,.004f);
    for(float sign:{-1.f,1.f}) {
        b.shard(sign,{{.148f,.034f},{.220f,.048f},{.249f,.024f},{.203f,-.003f}},.033f,.044f);
        b.shard(sign,{{.235f,.039f},{.314f,.025f},{.328f,-.004f},{.281f,-.012f}},.034f,.045f);
    }
    // One connected side channel wraps the raised stock plates. Its x
    // coordinates are outside the actual .044/.045 armor ridges, retaining
    // depth testing instead of drawing energy through the gun.
    for(float sign:{-1.f,1.f}) {
        std::vector<glm::vec3> route={{sign*.0559f,.023f,-.105f},{sign*.0559f,.004f,-.042f},
            {sign*.0559f,-.018f,-.005f},{sign*.0559f,.007f,.064f},{sign*.0559f,.010f,.114f},
            {sign*.0559f,.026f,.149f},{sign*.051f,.034f,.167f},{sign*.0465f,.021f,.183f},
            {sign*.0465f,-.001f,.258f},{sign*.0465f,-.024f,.297f},{sign*.0465f,-.095f,.315f}};
        b.ribbon(route,.010f,{sign,0,0},RedReflect);
        for(auto& p:route)p.x+=sign*.0003f;b.ribbon(route,.0057f,{sign,0,0},Energy);
        for(auto& p:route)p.x+=sign*.0002f;b.ribbon(route,.0025f,{sign,0,0},Core);
    }
    // The dorsal strip now crosses the rear bevel and joins the original cap
    // stripe with the same width/material. Shared endpoints and miter corners
    // prevent the old 4cm gap and separate-segment cracks at low resolution.
    std::vector<glm::vec3> back={{0,.0607f,.128f},{-.009f,.0697f,.178f},{.009f,.0737f,.213f},
        {-.006f,.0667f,.244f},{.010f,.0567f,.289f},{0,.0407f,.323f},{0,.022f,.343f}};
    std::vector<glm::vec3> normals(back.size(),glm::vec3(0,1,0));
    normals.back()=glm::normalize(glm::vec3(0,.55f,.85f));
    for(const auto xy:{glm::vec2(-.007f,.004f),glm::vec2(.007f,-.025f),glm::vec2(-.005f,-.057f),glm::vec2(.005f,-.089f)}) {
        const float z=.337f+(.322f-.337f)*((.011f-xy.y)/.127f)+.0075f;
        back.emplace_back(xy.x,xy.y,z);normals.emplace_back(0,0,1);
    }
    b.foldedRibbon(back,.012f,normals,RedReflect);
    for(size_t i=0;i<back.size();++i)back[i]+=normals[i]*.0004f;
    b.foldedRibbon(back,.0058f,normals,Energy);
    for(size_t i=0;i<back.size();++i)back[i]+=normals[i]*.0003f;
    b.foldedRibbon(back,.0025f,normals,Core);
    // Split muzzle: two pointed upper/lower jaws with an empty bore between.
    b.plate({{-.454f,.014f},{-.418f,.027f},{-.363f,.018f},{-.398f,.004f},{-.449f,.003f}},.021f,.003f);
    b.plate({{-.455f,-.023f},{-.399f,-.023f},{-.368f,-.012f},{-.399f,-.007f},{-.449f,-.012f}},.021f,.003f);
    for(float sign:{-1.f,1.f}) {
        const float x=sign*.025f;
        b.ribbon({{x,-.005f,-.451f},{x,-.004f,-.405f},{x,.001f,-.380f}},.0052f,{sign,0,0},Energy);
    }
    sight(b);
    return b.finish();
}
WeaponAsset resolveWeaponPresentation(const std::string& configPath,std::string& status) {
    // Config only chooses an explicit compatibility source. It cannot make the
    // normal gun depend on a downloaded mesh or a neighboring resource file.
    try {
        std::ifstream f(configPath,std::ios::binary);
        if(f) {
            std::string text;char buffer[16385];f.read(buffer,sizeof(buffer));
            if(f.gcount()>16384)throw std::runtime_error("weapon config exceeds 16KiB");
            text.assign(buffer,size_t(f.gcount()));const auto j=SimpleJson::parse(text);
            if(j.isObject()&&j.getString("source")=="imported") {
                WeaponAsset old;std::string reason;
                if(loadWeaponAsset(configPath,old,reason)) {appendCompatibilitySight(old);status="explicit legacy imported rifle";return old;}
                status="procedural Crimson Carapace (legacy import unavailable: "+reason+")";
                return buildProceduralRifle();
            }
        }
    }catch(const std::exception&) {status="procedural Crimson Carapace (invalid optional config ignored)";return buildProceduralRifle();}
    status="procedural Crimson Carapace (no external mesh or texture)";
    return buildProceduralRifle();
}
}
