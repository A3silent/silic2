#pragma once
#include "engine/map.h"
#include <functional>
#include <string>
#include <vector>

namespace silic2::editor {
enum class ObjectKind { None, Brush, Object, Entity, Light, Model, NavNode };
struct Selection {
    ObjectKind kind = ObjectKind::None;
    size_t index = 0;
    bool operator==(const Selection& other) const { return kind==other.kind && index==other.index; }
};
struct Bounds { glm::vec3 min{0},max{0}; };
struct DocumentState { Map map; Selection selection; uint64_t revision=0; };
class EditorDocument {
public:
    EditorDocument();
    const Map& map() const { return state.map; }
    Selection selection() const { return state.selection; }
    void select(Selection selected);
    bool selected() const;
    Bounds selectedBounds() const;
    glm::vec3 selectedPosition() const;
    glm::vec3 selectedSize() const;
    uint64_t revision() const { return state.revision; }
    bool dirty() const { return state.revision!=savedRevision; }
    const std::string& path() const { return currentPath; }
    const std::string& error() const { return lastError; }
    bool load(const std::string& path);
    bool save(const std::string& path);
    void createNew();
    bool createArena(const std::string& templatePath);
    bool applyJson(const std::string& label,const std::function<void(SimpleJson&)>& change,uint64_t mergeToken=0);
    SimpleJson selectedJson() const;
    bool editSelected(const std::string& label,const std::function<void(SimpleJson&)>& change,uint64_t mergeToken=0);
    void apply(const std::string& label,const std::function<void(Map&)>& change,uint64_t mergeToken=0);
    bool undo(); bool redo();
    bool canUndo() const { return cursor>0; }
    bool canRedo() const { return cursor<history.size(); }
    std::string undoLabel() const;
    std::string redoLabel() const;
    void transform(glm::vec3 center,glm::vec3 size,float snapStep=0,uint64_t mergeToken=0);
    void duplicate(float offset=1);
    void erase();
    void addBox(glm::vec3 center,glm::vec3 size);
    void addPrimitive(const std::string& shape,glm::vec3 center,glm::vec3 size);
    bool addModel(const std::string& asset,const std::string& collisionAsset,glm::vec3 position);
    void addNavNode(glm::vec3 position);
    bool connectNavNodes(int from,int to,bool bidirectional=true);
    void addEntity(EntityType type,glm::vec3 position);
    void addLight(glm::vec3 position);
    Selection pick(glm::vec3 origin,glm::vec3 direction,bool includeMarkers=true,bool includeNavigation=false,bool collision=false) const;
    static Bounds brushBounds(const Brush& brush);
    static glm::vec3 snap(glm::vec3 value,float step);
private:
    struct Change { DocumentState before,after; std::string label;uint64_t token=0; };
    DocumentState state;
    std::vector<Change> history;
    size_t cursor=0;
    uint64_t serial=0,savedRevision=0;
    std::string currentPath,lastError;
    void record(DocumentState before,const std::string& label,uint64_t token);
    uint32_t nextBrushId() const;
};
int measureEditorEdits(const std::string& fixture,const std::string& outputDirectory);
int runHybridSelfTests(const std::string& outputDirectory);
int runEditorSelfTests(const std::string& fixture,const std::string& outputDirectory);
} // namespace silic2::editor
