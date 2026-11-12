#include "engine/mesh_asset.h"
#include "engine/simple_json.h"
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <filesystem>
#include <limits>
namespace silic2 {
glm::mat4 staticTransform(glm::vec3 position,glm::vec3 rotation,glm::vec3 scale) {
    auto m=glm::translate(glm::mat4(1),position);
    m=glm::rotate(m,glm::radians(rotation.z),{0,0,1});
    m=glm::rotate(m,glm::radians(rotation.y),{0,1,0});
    m=glm::rotate(m,glm::radians(rotation.x),{1,0,0});
    return glm::scale(m,scale);
}
StaticMesh loadStaticObj(const std::string& path) {
    std::ifstream input(path); if(!input)throw std::runtime_error("Cannot open model "+path);
    std::vector<glm::vec3> positions,normals;std::vector<glm::vec2> uvs;StaticMesh result;
    std::string line;size_t lineNumber=0,totalBytes=0;
    auto fail=[&](const std::string& reason){throw std::runtime_error(path+":"+std::to_string(lineNumber)+": "+reason);};
    while(std::getline(input,line)) {
        ++lineNumber;totalBytes+=line.size();if(totalBytes>32*1024*1024)fail("model exceeds 32MB");
        std::istringstream row(line);std::string tag;row>>tag;if(tag.empty()||tag[0]=='#')continue;
        if(tag=="v" || tag=="vn") {
            glm::vec3 v;if(!(row>>v.x>>v.y>>v.z))fail("invalid vector");
            for(int i=0;i<3;++i)if(!std::isfinite(v[i])||std::abs(v[i])>100000)fail("invalid coordinate");
            (tag=="v"?positions:normals).push_back(v);
            if(positions.size()>200000 || normals.size()>200000)fail("too many vertices");
        } else if(tag=="vt") {
            glm::vec2 uv;if(!(row>>uv.x>>uv.y)||!std::isfinite(uv.x)||!std::isfinite(uv.y))fail("invalid UV");uvs.push_back(uv);
        } else if(tag=="f") {
            std::vector<uint32_t> face;std::string token;
            auto index=[&](const std::string& value,size_t count)->size_t {
                size_t used=0;long n=0;try{n=std::stol(value,&used);}catch(...){fail("invalid face index");}
                if(used!=value.size()||n==0)fail("invalid face index");
                long resolved=n>0?n-1:long(count)+n;if(resolved<0||resolved>=long(count))fail("face index out of range");return size_t(resolved);
            };
            while(row>>token) {
                if(token[0]=='#')break;
                auto a=token.find('/'),b=a==std::string::npos?a:token.find('/',a+1);
                auto pi=index(token.substr(0,a),positions.size());glm::vec2 uv(0);glm::vec3 authoredNormal(0);
                if(a!=std::string::npos) {
                    auto part=token.substr(a+1,b==std::string::npos?std::string::npos:b-a-1);
                    if(!part.empty())uv=uvs[index(part,uvs.size())];
                    if(b!=std::string::npos && b+1<token.size()) {
                        authoredNormal=normals[index(token.substr(b+1),normals.size())];
                        if(glm::length(authoredNormal)<1e-7f)fail("zero normal");
                        authoredNormal=glm::normalize(authoredNormal);
                    }
                }
                face.push_back(uint32_t(result.vertices.size()));result.vertices.push_back(positions[pi]);result.texCoords.push_back(uv);result.normals.push_back(authoredNormal);
            }
            if(face.size()!=3 && face.size()!=4)fail("export triangles or convex quads");
            if(face.size()==4) {
                const auto a=result.vertices[face[0]], b=result.vertices[face[1]], c=result.vertices[face[2]], d=result.vertices[face[3]];
                auto normal=glm::cross(b-a,c-a);const float area=glm::length(normal);
                if(area<1e-7f)fail("degenerate quad");
                normal/=area;
                const float extent=std::max(std::max(glm::length(b-a),glm::length(c-a)),glm::length(d-a));
                if(std::abs(glm::dot(d-a,normal))>std::max(1e-5f,extent*1e-5f))fail("non-planar quad; triangulate in DCC");
                for(size_t j=0;j<4;++j) {
                    auto u=result.vertices[face[(j+1)%4]]-result.vertices[face[j]];
                    auto v=result.vertices[face[(j+2)%4]]-result.vertices[face[(j+1)%4]];
                    if(glm::dot(glm::cross(u,v),normal)<=1e-7f)fail("non-convex quad; triangulate in DCC");
                }
            }
            for(size_t i=1;i+1<face.size();++i) {
                auto a=result.vertices[face[0]],b=result.vertices[face[i]],c=result.vertices[face[i+1]];
                if(glm::length(glm::cross(b-a,c-a))<1e-7f)fail("degenerate triangle");
                for(auto index:{face[0],face[i],face[i+1]})
                    if(glm::length(result.normals[index])<1e-7f)result.normals[index]=glm::normalize(glm::cross(b-a,c-a));
                result.faces.insert(result.faces.end(),{face[0],face[i],face[i+1]});
            }
            if(result.faces.size()>300000)fail("model exceeds 100000 triangles");
        } else if(tag!="o" && tag!="g" && tag!="s" && tag!="usemtl" && tag!="mtllib")fail("unsupported OBJ record "+tag);
    }
    if(result.faces.empty())throw std::runtime_error("Model contains no faces: "+path);
    return result;
}

StaticModelBundle loadStaticModelBundle(const std::string& manifestPath) {
    namespace fs=std::filesystem;
    auto fail=[](const std::string& message){throw std::runtime_error("Static model bundle: "+message);};
    std::ifstream stream(manifestPath,std::ios::binary);
    if(!stream)fail("cannot open manifest "+manifestPath);
    if(fs::file_size(manifestPath)>1024*1024)fail("manifest exceeds 1 MiB");
    const std::string text((std::istreambuf_iterator<char>(stream)),{});
    if(text.size()>1024*1024)fail("manifest exceeds 1 MiB");
    const auto j=SimpleJson::parse(text);
    if(!j.isObject() || j.getString("format")!="silic2-static-model" ||
       !j["schemaVersion"].isNumber() || j["schemaVersion"].getNumber()!=1)fail("unsupported format/version");
    const auto root=fs::canonical(fs::absolute(manifestPath).parent_path());
    auto resource=[&](const SimpleJson& value,bool required)->std::string {
        if(!required && value.isNull())return {};
        if(!value.isString())fail("resource path must be a string");
        const auto name=value.getString();
        if(!required && name.empty())return {};
        const fs::path relative(name);
        if(name.empty() || relative.is_absolute() || name.find(':')!=std::string::npos)fail("resource path must be relative");
        const auto resolved=fs::weakly_canonical(root/relative);
        auto ri=root.begin(), pi=resolved.begin();
        for(;ri!=root.end();++ri,++pi)if(pi==resolved.end() || *ri!=*pi)fail("resource escapes bundle directory");
        if(!fs::is_regular_file(resolved))fail("resource missing: "+name);
        return resolved.string();
    };
    auto vector=[&](const SimpleJson& value)->glm::vec3 {
        if(!value.isArray() || value.size()!=3)fail("invalid bounds vector");
        glm::vec3 out;
        for(size_t i=0;i<3;++i){if(!value[i].isNumber() || !std::isfinite(value[i].getNumber()) || std::abs(value[i].getNumber())>100000)fail("invalid bounds coordinate");out[i]=float(value[i].getNumber());}
        return out;
    };
    StaticModelBundle bundle;
    const auto declaredMin=vector(j["bounds"]["min"]), declaredMax=vector(j["bounds"]["max"]);
    for(int i=0;i<3;++i)if(declaredMin[i]>declaredMax[i])fail("inverted bounds");
    const auto& parts=j["parts"];
    if(!parts.isArray() || parts.size()==0 || parts.size()>128)fail("expected 1..128 material parts");
    bundle.boundsMin=glm::vec3(std::numeric_limits<float>::max());
    bundle.boundsMax=glm::vec3(std::numeric_limits<float>::lowest());
    size_t triangles=0;
    for(size_t i=0;i<parts.size();++i) {
        const auto& descriptor=parts[i];
        if(!descriptor.isObject())fail("invalid part");
        StaticModelPart part;part.asset=resource(descriptor["obj"],true);part.texture=resource(descriptor["baseColorTexture"],false);
        if(fs::path(part.asset).extension()!=".obj")fail("part must reference an OBJ");
        if(!part.texture.empty()) {
            const auto ext=fs::path(part.texture).extension();
            if(ext!=".png" && ext!=".jpg" && ext!=".jpeg")fail("texture must be PNG/JPEG");
            if(fs::file_size(part.texture)>64*1024*1024)fail("texture exceeds 64 MiB");
        }
        const auto& color=descriptor["baseColorFactor"];
        if(!color.isArray() || color.size()!=4)fail("baseColorFactor requires four components");
        for(size_t c=0;c<4;++c){if(!color[c].isNumber() || !std::isfinite(color[c].getNumber()) || color[c].getNumber()<0 || color[c].getNumber()>1)fail("invalid baseColorFactor");part.baseColorFactor[c]=float(color[c].getNumber());}
        if(part.baseColorFactor.a!=1.f)fail("only opaque materials supported");
        part.mesh=loadStaticObj(part.asset);
        triangles+=part.mesh.faces.size()/3;if(triangles>100000)fail("bundle exceeds 100000 triangles");
        for(const auto& v:part.mesh.vertices){bundle.boundsMin=glm::min(bundle.boundsMin,v);bundle.boundsMax=glm::max(bundle.boundsMax,v);}
        bundle.parts.push_back(std::move(part));
    }
    for(int i=0;i<3;++i) {
        const float tolerance=std::max(1e-5f,std::max(std::abs(declaredMin[i]),std::abs(declaredMax[i]))*1e-5f);
        if(std::abs(bundle.boundsMin[i]-declaredMin[i])>tolerance || std::abs(bundle.boundsMax[i]-declaredMax[i])>tolerance)fail("manifest bounds do not match geometry");
    }
    return bundle;
}
}
