#pragma once
#include "engine/map.h"
#include "engine/collision.h"
#include <algorithm>
#include <cmath>

namespace silic2::enemyGeometry {
inline AABB bounds(const Brush& brush) {
    glm::vec3 low=brush.vertices.front(), high=low;
    for(const auto& v:brush.vertices) { low=glm::min(low,v); high=glm::max(high,v); }
    return {low,high};
}
// Returns the first fraction on a closed segment, including starting overlap.
inline bool segment(const glm::vec3& start,const glm::vec3& end,const AABB& box,float& fraction) {
    float lo=0,hi=1; const auto delta=end-start;
    for(int axis=0;axis<3;++axis) {
        if(std::abs(delta[axis])<1e-8f) {
            if(start[axis]<box.min[axis] || start[axis]>box.max[axis]) return false;
        } else {
            float a=(box.min[axis]-start[axis])/delta[axis],b=(box.max[axis]-start[axis])/delta[axis];
            if(a>b) std::swap(a,b);
            lo=std::max(lo,a); hi=std::min(hi,b);
            if(lo>hi) return false;
        }
    }
    fraction=lo; return true;
}
inline bool clear(const glm::vec3& start,const glm::vec3& end,const Map* map,float radius=0) {
    if(!map) return true;
    if(auto world=map->getSpatialWorld())return !world->raycast(start,end,radius).hit;
    for(const auto& brush:map->getBrushes()) {
        if(brush.vertices.empty()) continue;
        auto box=bounds(brush); box.min-=glm::vec3(radius); box.max+=glm::vec3(radius);
        float fraction=0; if(segment(start,end,box,fraction)) return false;
    }
    return true;
}
}
