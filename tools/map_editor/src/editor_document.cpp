#include "editor_document.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <chrono>

namespace silic2::editor {
namespace fs=std::filesystem;
EditorDocument::EditorDocument() { state.revision=++serial;savedRevision=state.revision; }
void EditorDocument::select(Selection selected) { state.selection=selected;if(!this->selected()) state.selection={}; }
bool EditorDocument::selected() const {
    switch(state.selection.kind) {
        case ObjectKind::Brush:return state.selection.index<state.map.getLegacyBrushes().size();
        case ObjectKind::Object:return state.selection.index<state.map.toJson()["geometry"]["objects"].size();
        case ObjectKind::Entity:return state.selection.index<state.map.getEntities().size();
        case ObjectKind::Light:return state.selection.index<state.map.getLights().size();
        default:return false;
    }
}
Bounds EditorDocument::brushBounds(const Brush& b) {
    if(b.vertices.empty()) return {};
    Bounds result{b.vertices[0],b.vertices[0]};
    for(auto p:b.vertices) { result.min=glm::min(result.min,p);result.max=glm::max(result.max,p); }
    return result;
}
Bounds EditorDocument::selectedBounds() const {
    if(!selected()) return {};
    auto i=state.selection.index;
    if(state.selection.kind==ObjectKind::Brush) return brushBounds(state.map.getLegacyBrushes()[i]);
    if(state.selection.kind==ObjectKind::Object) {
        auto j=selectedJson();glm::vec3 p(0),s(0);for(int a=0;a<3;++a){p[a]=j["position"][a].getNumber();s[a]=j["size"][a].getNumber();}return {p-s*.5f,p+s*.5f};
    }
    if(state.selection.kind==ObjectKind::Entity) {
        const auto& e=state.map.getEntities()[i];const auto r=glm::max(glm::abs(e.scale),glm::vec3(.1f))*.25f;
        return {e.position-r,e.position+r};
    }
    auto p=state.map.getLights()[i].position;return {p-glm::vec3(.2f),p+glm::vec3(.2f)};
}
bool EditorDocument::load(const std::string& path) {
    Map candidate;
    if(!candidate.loadFromFile(path)) { lastError=candidate.getLastError()+" Current work is unchanged.";return false; }
    state.map=candidate;state.selection={};state.revision=++serial;
    savedRevision=state.revision;history.clear();cursor=0;currentPath=fs::absolute(path).lexically_normal().string();lastError.clear();return true;
}
bool EditorDocument::save(const std::string& path) {
    if(path.empty()) { lastError="Choose a file name first.";return false; }
    fs::path destination=fs::absolute(path).lexically_normal();
    const auto nonce=std::chrono::steady_clock::now().time_since_epoch().count();
    fs::path temporary=destination.parent_path()/("."+destination.filename().string()+".saving-"+std::to_string(nonce)+".json");
    try {
        if(!state.map.saveToFile(temporary.string())) { std::error_code ignored;fs::remove(temporary,ignored);lastError=state.map.getLastError();return false; }
        fs::rename(temporary,destination);
        currentPath=destination.string();savedRevision=state.revision;lastError.clear();return true;
    } catch(const std::exception& e) {
        std::error_code ignored;fs::remove(temporary,ignored);lastError=std::string("Save failed: ")+e.what();return false;
    }
}
namespace {
SimpleJson vectorJson(glm::vec3 v) { SimpleJson a=SimpleJson::parse("[]");for(int i=0;i<3;++i)a.push_back(SimpleJson(double(v[i])));return a; }
glm::vec3 vectorValue(const SimpleJson& a) { return {a[0].getNumber(),a[1].getNumber(),a[2].getNumber()}; }
SimpleJson& selectedArray(SimpleJson& root,ObjectKind kind) {
    if(kind==ObjectKind::Brush)return root["geometry"]["brushes"];
    if(kind==ObjectKind::Object)return root["geometry"]["objects"];
    if(kind==ObjectKind::Entity)return root["entities"];
    return root["lights"];
}
}
void EditorDocument::createNew() {
    Map candidate;
    auto root=SimpleJson::parse(R"({"schemaVersion":2,"gameMode":"legacy","environmentPreset":"none","worldSettings":{"gravity":-9.8},"geometry":{"brushes":[],"objects":[{"id":1,"name":"Floor","shape":"box","position":[0,-0.25,0],"size":[20,0.5,20],"type":"floor","material":"stone","color":[0.38,0.48,0.58]}]},"entities":[{"type":"player_start","name":"Player Start","position":[0,1.5,5]}],"lights":[]})");
    if(!candidate.loadFromJson(root)){lastError=candidate.getLastError();return;}
    state.map=candidate;state.selection={};history.clear();cursor=0;currentPath.clear();
    state.revision=++serial;savedRevision=0;lastError.clear();
}
bool EditorDocument::createArena(const std::string& path) {
    if(!load(path))return false;currentPath.clear();savedRevision=0;return true;
}
SimpleJson EditorDocument::selectedJson() const {
    if(!selected())return SimpleJson::parse("{}");
    auto root=state.map.toJson();return selectedArray(root,state.selection.kind)[state.selection.index];
}
bool EditorDocument::applyJson(const std::string& label,const std::function<void(SimpleJson&)>& change,uint64_t token) {
    auto json=state.map.toJson();const auto original=json.toString();change(json);
    if(original==json.toString())return false;
    Map candidate;
    if(!candidate.loadFromJson(json)){lastError=candidate.getLastError();return false;}
    auto before=state;state.map=candidate;record(std::move(before),label,token);lastError.clear();return true;
}
bool EditorDocument::editSelected(const std::string& label,const std::function<void(SimpleJson&)>& change,uint64_t token) {
    if(!selected())return false;
    return applyJson(label,[&](SimpleJson& root){change(selectedArray(root,state.selection.kind)[state.selection.index]);},token);
}
void EditorDocument::record(DocumentState before,const std::string& label,uint64_t token) {
    state.revision=++serial;
    if(token && cursor==history.size() && cursor && history.back().token==token && before.revision!=savedRevision) {
        history.back().after=state;return;
    }
    history.resize(cursor);history.push_back({std::move(before),state,label,token});cursor=history.size();
    if(history.size()>128) { history.erase(history.begin());--cursor; }
}
void EditorDocument::apply(const std::string& label,const std::function<void(Map&)>& change,uint64_t token) {
    Map candidate=state.map;change(candidate);
    applyJson(label,[&](SimpleJson& root){root=candidate.toJson();},token);
}
bool EditorDocument::undo() { if(!canUndo()) return false;state=history[--cursor].before;lastError.clear();return true; }
bool EditorDocument::redo() { if(!canRedo()) return false;state=history[cursor++].after;lastError.clear();return true; }
std::string EditorDocument::undoLabel() const { return canUndo()?history[cursor-1].label:""; }
std::string EditorDocument::redoLabel() const { return canRedo()?history[cursor].label:""; }
glm::vec3 EditorDocument::snap(glm::vec3 p,float step) { return step>0 ? glm::round(p/step)*step:p; }
void EditorDocument::transform(glm::vec3 center,glm::vec3 size,float step,uint64_t token) {
    if(!selected()) return;
    center=snap(center,step);Bounds old=selectedBounds();
    const glm::vec3 oldCenter=(old.min+old.max)*.5f,oldSize=old.max-old.min;
    size=glm::max(snap(size,step),glm::vec3(0));
    editSelected("Transform object",[&](SimpleJson& j){
        if(state.selection.kind==ObjectKind::Brush) {
            for(size_t i=0;i+2<j["vertices"].size();i+=3) {
                glm::vec3 p(j["vertices"][i].getNumber(),j["vertices"][i+1].getNumber(),j["vertices"][i+2].getNumber());
                for(int a=0;a<3;++a)p[a]=center[a]+(oldSize[a]>.0001f?(p[a]-oldCenter[a])*std::max(size[a],.01f)/oldSize[a]:0);
                for(int a=0;a<3;++a)j["vertices"][i+a]=SimpleJson(double(p[a]));
            }
        } else if(state.selection.kind==ObjectKind::Object) {
            j["position"]=vectorJson(center);
            auto dimensions=glm::max(size,glm::vec3(.01f));if(j.getString("shape")=="plane")dimensions.y=0;
            j["size"]=vectorJson(dimensions);
        } else if(state.selection.kind==ObjectKind::Entity) {
            j["position"]=vectorJson(center);j["scale"]=vectorJson(glm::max(size*2.0f,glm::vec3(.01f)));
        } else {
            if(j.hasKey("anchor")) {
                glm::vec3 offset(0);if(j["anchor"].hasKey("offset"))offset=vectorValue(j["anchor"]["offset"]);
                j["anchor"]["offset"]=vectorJson(offset+center-oldCenter);
            } else j["position"]=vectorJson(center);
        }
    },token);
}
uint32_t EditorDocument::nextBrushId() const { uint32_t value=0;for(const auto& b:state.map.getBrushes())value=std::max(value,b.id);return value+1; }
void EditorDocument::duplicate(float offset) {
    if(!selected())return;
    const auto kind=state.selection.kind;const auto source=selectedJson();size_t newIndex=0;
    const bool changed=applyJson("Duplicate object",[&](SimpleJson& root){
        auto copy=source;const glm::vec3 delta(offset,0,offset);
        if(kind==ObjectKind::Brush || kind==ObjectKind::Object)copy["id"]=SimpleJson(double(nextBrushId()));
        if(kind==ObjectKind::Brush){for(size_t i=0;i+2<copy["vertices"].size();i+=3)for(int a=0;a<3;++a)copy["vertices"][i+a]=SimpleJson(copy["vertices"][i+a].getNumber()+delta[a]);}
        else if(kind==ObjectKind::Light && copy.hasKey("anchor"))copy["anchor"]["offset"]=vectorJson(vectorValue(copy["anchor"]["offset"])+delta);
        else copy["position"]=vectorJson(vectorValue(copy["position"])+delta);
        if(copy.hasKey("name"))copy["name"]=SimpleJson(copy.getString("name")+" Copy "+std::to_string(serial+1));
        auto& array=selectedArray(root,kind);newIndex=array.size();array.push_back(copy);
    });
    if(changed){state.selection={kind,newIndex};history.back().after.selection=state.selection;}
}
void EditorDocument::erase() {
    if(!selected())return;const auto selected=state.selection;
    const bool changed=applyJson("Delete object",[&](SimpleJson& root){
        auto& array=selectedArray(root,selected.kind);auto kept=SimpleJson::parse("[]");
        for(size_t i=0;i<array.size();++i)if(i!=selected.index)kept.push_back(array[i]);array=kept;
    });
    if(changed){state.selection={};history.back().after.selection={};}
}
void EditorDocument::addBox(glm::vec3 center,glm::vec3 size) {
    size=glm::max(size,glm::vec3(.01f));size_t index=0;
    const bool changed=applyJson("Add box",[&](SimpleJson& root){
        root["schemaVersion"]=SimpleJson(2.0);auto object=SimpleJson::parse(R"({"name":"Box","shape":"box","material":"stone","type":"wall","color":[0.38,0.48,0.58]})");
        object["id"]=SimpleJson(double(nextBrushId()));object["position"]=vectorJson(center);object["size"]=vectorJson(size);
        auto& array=root["geometry"]["objects"];if(!array.isArray())array=SimpleJson::parse("[]");index=array.size();array.push_back(object);
    });
    if(changed){state.selection={ObjectKind::Object,index};history.back().after.selection=state.selection;}
}
void EditorDocument::addEntity(EntityType type,glm::vec3 position) {
    static const char* names[]={"player_start","light","enemy_spawn","item_pickup","trigger","rift","sentry_spawn"};
    size_t index=0;
    const bool changed=applyJson("Add entity",[&](SimpleJson& root){
        auto e=SimpleJson::parse("{}");e["type"]=SimpleJson(names[static_cast<int>(type)]);e["name"]=SimpleJson("Entity "+std::to_string(serial+1));e["position"]=vectorJson(position);
        auto& array=root["entities"];if(!array.isArray())array=SimpleJson::parse("[]");index=array.size();array.push_back(e);
    });
    if(changed){state.selection={ObjectKind::Entity,index};history.back().after.selection=state.selection;}
}
void EditorDocument::addLight(glm::vec3 position) {
    size_t index=0;
    const bool changed=applyJson("Add light",[&](SimpleJson& root){
        auto light=SimpleJson::parse(R"({"type":"point","color":[1,1,1],"intensity":1,"range":10})");light["position"]=vectorJson(position);
        auto& array=root["lights"];if(!array.isArray())array=SimpleJson::parse("[]");index=array.size();array.push_back(light);
    });
    if(changed){state.selection={ObjectKind::Light,index};history.back().after.selection=state.selection;}
}
namespace {
float rayBox(glm::vec3 origin,glm::vec3 direction,Bounds box) {
    float near=0,far=std::numeric_limits<float>::infinity();
    for(int a=0;a<3;++a) {
        if(std::abs(direction[a])<1e-7f) { if(origin[a]<box.min[a] || origin[a]>box.max[a])return -1;continue; }
        float lo=(box.min[a]-origin[a])/direction[a],hi=(box.max[a]-origin[a])/direction[a];if(lo>hi)std::swap(lo,hi);
        near=std::max(near,lo);far=std::min(far,hi);if(near>far)return -1;
    } return near;
}
float rayTriangle(glm::vec3 o,glm::vec3 d,glm::vec3 a,glm::vec3 b,glm::vec3 c) {
    auto e1=b-a,e2=c-a,p=glm::cross(d,e2);float det=glm::dot(e1,p);if(std::abs(det)<1e-7f)return -1;
    auto t=o-a;float u=glm::dot(t,p)/det;if(u<0 || u>1)return -1;
    auto q=glm::cross(t,e1);float v=glm::dot(d,q)/det;if(v<0 || u+v>1)return -1;
    float distance=glm::dot(e2,q)/det;return distance>=0?distance:-1;
}
}
Selection EditorDocument::pick(glm::vec3 origin,glm::vec3 direction,bool markers) const {
    Selection selected;float distance=std::numeric_limits<float>::infinity();
    auto consider=[&](float hit,Selection selection){if(hit>=0 && hit<distance){distance=hit;selected=selection;}};
    const auto& brushes=state.map.getBrushes();
    const auto objects=state.map.toJson()["geometry"]["objects"];
    for(size_t i=0;i<brushes.size();++i) {
        const auto& b=brushes[i];
        for(size_t f=0;f+2<b.faces.size();f+=3) {
            auto a=b.faces[f],c=b.faces[f+1],e=b.faces[f+2];
            if(a<b.vertices.size() && c<b.vertices.size() && e<b.vertices.size()){
                Selection authored;
                const auto& legacy=state.map.getLegacyBrushes();
                for(size_t j=0;j<legacy.size();++j)if(legacy[j].id==b.id)authored={ObjectKind::Brush,j};
                for(size_t j=0;j<objects.size();++j)if(objects[j].getNumber("id")==b.id)authored={ObjectKind::Object,j};
                consider(rayTriangle(origin,direction,b.vertices[a],b.vertices[c],b.vertices[e]),authored);
            }
        }
    }
    if(markers) {
        const auto& entities=state.map.getEntities();for(size_t i=0;i<entities.size();++i) {
            auto p=entities[i].position,r=glm::max(glm::abs(entities[i].scale),glm::vec3(.1f))*.25f;
            consider(rayBox(origin,direction,{p-r,p+r}),{ObjectKind::Entity,i});
        }
        const auto& lights=state.map.getLights();for(size_t i=0;i<lights.size();++i) {
            auto p=lights[i].position;consider(rayBox(origin,direction,{p-glm::vec3(.2f),p+glm::vec3(.2f)}),{ObjectKind::Light,i});
        }
    } return selected;
}
} // namespace silic2::editor
