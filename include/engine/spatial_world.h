#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <vector>
namespace silic2 {
struct Brush;
struct SpatialNavNode { uint32_t id=0; glm::vec3 position{0}; };
struct SpatialNavLink { uint32_t from=0,to=0; bool bidirectional=true; };
struct SpatialHit {
    bool hit=false; float fraction=1; glm::vec3 position{0},normal{0,1,0}; uint32_t objectId=0;
};
struct GroundSupport {
    bool hit=false; float height=0; glm::vec3 normal{0,1,0}; uint32_t objectId=0;
};
struct ActorMove { glm::vec3 position{0}; bool grounded=false,ceiling=false; };
struct SpatialTriangle { glm::vec3 a,b,c,normal; uint32_t objectId=0; };
// Immutable static world: triangle surfaces plus an explicitly connected 3D navigation graph.
// Bounds only prune candidate triangles; they never replace surface/opening tests.
class SpatialWorld {
public:
    SpatialWorld(const std::vector<Brush>& collision,const std::vector<SpatialNavNode>& nodes,const std::vector<SpatialNavLink>& links);
    const std::vector<SpatialTriangle>& triangles() const { return surfaces; }
    SpatialHit raycast(glm::vec3 start,glm::vec3 end,float radius=0) const;
    GroundSupport support(glm::vec3 feet,float maxAbove,float maxBelow,float radius=0) const;
    ActorMove moveActor(glm::vec3 feet,glm::vec3 displacement,float radius,float height,bool grounded,float stepHeight=.3f) const;
    bool isWalkable(glm::vec3 feet,float radius=.34f,float height=1.8f) const;
    bool canTraverse(glm::vec3 from,glm::vec3 to,float radius=.34f,float height=1.8f) const;
    glm::vec3 nextWaypoint(glm::vec3 from,glm::vec3 to,float radius=.34f) const;
    bool navigationConnected() const;
    std::vector<std::string> diagnostics() const;
private:
    struct Bounds { glm::vec3 minimum{0},maximum{0}; };
    struct Node { Bounds box; int left=-1,right=-1; size_t begin=0,end=0; };
    std::vector<SpatialTriangle> surfaces;
    std::vector<size_t> order;
    std::vector<Node> tree;
    std::vector<SpatialNavNode> navNodes;
    std::vector<SpatialNavLink> navLinks;
    std::vector<std::vector<float>> edges;
    mutable bool targetCached=false;
    mutable glm::vec3 cachedTarget{0};
    mutable float cachedRadius=0;
    mutable std::vector<float> targetCosts;
    mutable std::vector<int> targetNext;
    int buildTree(size_t begin,size_t end);
    std::vector<size_t> candidates(glm::vec3 minimum,glm::vec3 maximum) const;
    bool actorClear(glm::vec3 feet,float radius,float height) const;
    bool insideSolid(glm::vec3 point) const;
    struct SolidBounds {uint32_t id; Bounds box;};
    std::vector<SolidBounds> solidBounds;
};
}
