#include "engine/spatial_world.h"
#include "engine/map.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <queue>
#include <unordered_map>
namespace silic2 {
namespace {
constexpr float slopeCos=.70710678f, skin=.015f;
bool overlaps(glm::vec3 a,glm::vec3 b,glm::vec3 c,glm::vec3 d) {
    return a.x<=d.x&&b.x>=c.x&&a.y<=d.y&&b.y>=c.y&&a.z<=d.z&&b.z>=c.z;
}
glm::vec3 closestTriangle(glm::vec3 p,const SpatialTriangle& t) {
    auto ab=t.b-t.a,ac=t.c-t.a,ap=p-t.a;float d1=glm::dot(ab,ap),d2=glm::dot(ac,ap);
    if(d1<=0&&d2<=0)return t.a;
    auto bp=p-t.b;float d3=glm::dot(ab,bp),d4=glm::dot(ac,bp);if(d3>=0&&d4<=d3)return t.b;
    float vc=d1*d4-d3*d2;if(vc<=0&&d1>=0&&d3<=0)return t.a+ab*(d1/(d1-d3));
    auto cp=p-t.c;float d5=glm::dot(ab,cp),d6=glm::dot(ac,cp);if(d6>=0&&d5<=d6)return t.c;
    float vb=d5*d2-d1*d6;if(vb<=0&&d2>=0&&d6<=0)return t.a+ac*(d2/(d2-d6));
    float va=d3*d6-d5*d4;if(va<=0&&(d4-d3)>=0&&(d5-d6)>=0)return t.b+(t.c-t.b)*((d4-d3)/((d4-d3)+(d5-d6)));
    float inverse=1/(va+vb+vc);return t.a+ab*(vb*inverse)+ac*(vc*inverse);
}
bool intersect(glm::vec3 start,glm::vec3 end,const SpatialTriangle& tri,float& fraction) {
    auto direction=end-start,e1=tri.b-tri.a,e2=tri.c-tri.a,p=glm::cross(direction,e2);float determinant=glm::dot(e1,p);
    if(std::abs(determinant)<1e-9f)return false;
    float inverse=1/determinant;auto t=start-tri.a;float u=glm::dot(t,p)*inverse;if(u<-.00001f||u>1.00001f)return false;
    auto q=glm::cross(t,e1);float v=glm::dot(direction,q)*inverse;if(v<-.00001f||u+v>1.00001f)return false;
    fraction=glm::dot(e2,q)*inverse;return fraction>=0&&fraction<=1;
}
void closestSegments(glm::vec3 p,glm::vec3 q,glm::vec3 a,glm::vec3 b,glm::vec3& onFirst,glm::vec3& onSecond) {
    auto d1=q-p,d2=b-a,r=p-a;float aa=glm::dot(d1,d1),ee=glm::dot(d2,d2),f=glm::dot(d2,r),s=0,t=0;
    if(aa<=1e-9f)t=glm::clamp(f/ee,0.f,1.f);
    else {float c=glm::dot(d1,r);if(ee<=1e-9f)s=glm::clamp(-c/aa,0.f,1.f);
        else {float bb=glm::dot(d1,d2),den=aa*ee-bb*bb;if(den!=0)s=glm::clamp((bb*f-c*ee)/den,0.f,1.f);t=(bb*s+f)/ee;if(t<0){t=0;s=glm::clamp(-c/aa,0.f,1.f);}else if(t>1){t=1;s=glm::clamp((bb-c)/aa,0.f,1.f);}}}
    onFirst=p+d1*s;onSecond=a+d2*t;
}
float capsuleDistance(glm::vec3 a,glm::vec3 b,const SpatialTriangle& tri,glm::vec3& onActor,glm::vec3& onSurface) {
    float t;if(intersect(a,b,tri,t)){onActor=onSurface=glm::mix(a,b,t);return 0;}
    onActor=a;onSurface=closestTriangle(a,tri);float best=glm::length(onActor-onSurface);
    auto check=[&](glm::vec3 p,glm::vec3 q){float d=glm::length(p-q);if(d<best){best=d;onActor=p;onSurface=q;}};
    check(b,closestTriangle(b,tri));glm::vec3 p,q;
    closestSegments(a,b,tri.a,tri.b,p,q);check(p,q);closestSegments(a,b,tri.b,tri.c,p,q);check(p,q);closestSegments(a,b,tri.c,tri.a,p,q);check(p,q);return best;
}
bool supportingFace(const SpatialTriangle& t,glm::vec3 feet,glm::vec3 nearest) {
    return t.normal.y>=slopeCos && glm::dot(feet+glm::vec3(0,.3f,0)-t.a,t.normal)>=0 && feet.y>=nearest.y-.4f;
}
}
SpatialWorld::SpatialWorld(const std::vector<Brush>& collision,const std::vector<SpatialNavNode>& nodes,const std::vector<SpatialNavLink>& links):navNodes(nodes),navLinks(links) {
    for(const auto& b:collision)for(size_t i=0;i+2<b.faces.size();i+=3) {
        SpatialTriangle t{b.vertices[b.faces[i]],b.vertices[b.faces[i+1]],b.vertices[b.faces[i+2]],{},b.id};
        auto n=glm::cross(t.b-t.a,t.c-t.a);if(glm::length(n)<1e-7f)continue;t.normal=glm::normalize(n);surfaces.push_back(t);
    }
    for(const auto& b:collision)if(!b.vertices.empty()){Bounds box{b.vertices.front(),b.vertices.front()};for(auto v:b.vertices){box.minimum=glm::min(box.minimum,v);box.maximum=glm::max(box.maximum,v);}solidBounds.push_back({b.id,box});}
    order.resize(surfaces.size());std::iota(order.begin(),order.end(),0);if(!order.empty())buildTree(0,order.size());
    edges.assign(nodes.size(),std::vector<float>(nodes.size(),std::numeric_limits<float>::infinity()));
    std::unordered_map<uint32_t,size_t> indices;for(size_t i=0;i<nodes.size();++i){indices[nodes[i].id]=i;edges[i][i]=0;}
    for(const auto& link:links) {
        auto a=indices.find(link.from),b=indices.find(link.to);if(a==indices.end()||b==indices.end())continue;
        if(canTraverse(nodes[a->second].position,nodes[b->second].position,.65f)) {
            float cost=glm::length(nodes[a->second].position-nodes[b->second].position);edges[a->second][b->second]=cost;if(link.bidirectional)edges[b->second][a->second]=cost;
        }
    }
}
int SpatialWorld::buildTree(size_t begin,size_t end) {
    Node node;node.begin=begin;node.end=end;node.box.minimum=glm::vec3(std::numeric_limits<float>::max());node.box.maximum=-node.box.minimum;
    for(size_t i=begin;i<end;++i){const auto& t=surfaces[order[i]];node.box.minimum=glm::min(node.box.minimum,glm::min(t.a,glm::min(t.b,t.c)));node.box.maximum=glm::max(node.box.maximum,glm::max(t.a,glm::max(t.b,t.c)));}
    int index=int(tree.size());tree.push_back(node);
    if(end-begin>8){auto size=node.box.maximum-node.box.minimum;int axis=size.x>size.y?0:1;if(size.z>size[axis])axis=2;size_t mid=(begin+end)/2;
        std::nth_element(order.begin()+begin,order.begin()+mid,order.begin()+end,[&](size_t a,size_t b){const auto& x=surfaces[a];const auto& y=surfaces[b];return x.a[axis]+x.b[axis]+x.c[axis]<y.a[axis]+y.b[axis]+y.c[axis];});
        int left=buildTree(begin,mid),right=buildTree(mid,end);tree[index].left=left;tree[index].right=right;
    }return index;
}
std::vector<size_t> SpatialWorld::candidates(glm::vec3 minimum,glm::vec3 maximum) const {
    std::vector<size_t> result;if(tree.empty())return result;std::vector<int> stack{0};
    while(!stack.empty()){int index=stack.back();stack.pop_back();const auto& n=tree[index];if(!overlaps(minimum,maximum,n.box.minimum,n.box.maximum))continue;
        if(n.left>=0){stack.push_back(n.left);stack.push_back(n.right);}else for(size_t i=n.begin;i<n.end;++i)result.push_back(order[i]);}
    return result;
}
SpatialHit SpatialWorld::raycast(glm::vec3 start,glm::vec3 end,float radius) const {
    SpatialHit result;float length=glm::length(end-start);if(length<1e-8f || radius<0)return result;
    for(size_t i:candidates(glm::min(start,end)-glm::vec3(radius),glm::max(start,end)+glm::vec3(radius))) {
        const auto& tri=surfaces[i];float fraction=0;bool hit=false;glm::vec3 normal=tri.normal;
        if(radius<=1e-6f)hit=intersect(start,end,tri,fraction);
        else for(int step=0;step<64 && fraction<=1;++step) {
            auto center=glm::mix(start,end,fraction),point=closestTriangle(center,tri);float distance=glm::length(center-point);
            if(distance<=radius+1e-5f){hit=true;if(distance>1e-6f)normal=(center-point)/distance;break;}
            fraction+=(distance-radius)/length;
        }
        if(hit && (!result.hit || fraction<result.fraction)){result={true,fraction,glm::mix(start,end,fraction),normal,tri.objectId};}
    }return result;
}
GroundSupport SpatialWorld::support(glm::vec3 feet,float above,float below,float radius) const {
    GroundSupport result;glm::vec3 top=feet+glm::vec3(0,above+.0001f,0),bottom=feet-glm::vec3(0,below+.0001f,0);
    for(size_t i:candidates(bottom-glm::vec3(radius,0,radius),top+glm::vec3(radius,0,radius))) {
        const auto& tri=surfaces[i];if(tri.normal.y<slopeCos)continue;
        const glm::vec3 offsets[]={{0,0,0},{radius*.7f,0,0},{-radius*.7f,0,0},{0,0,radius*.7f},{0,0,-radius*.7f}};
        for(auto offset:offsets){float t;if(intersect(top+offset,bottom+offset,tri,t)){float h=glm::mix(top,bottom,t).y;if(!result.hit||h>result.height)result={true,h,tri.normal,tri.objectId};}}
    }return result;
}
bool SpatialWorld::insideSolid(glm::vec3 point) const {
    std::vector<uint32_t> eligible;for(const auto& solid:solidBounds)if(overlaps(point,point,solid.box.minimum+glm::vec3(.001f),solid.box.maximum-glm::vec3(.001f)))eligible.push_back(solid.id);
    if(eligible.empty())return false;
    const auto end=point+glm::vec3(431,731,529)*1000.f;std::unordered_map<uint32_t,std::pair<float,float>> first;
    for(size_t i:candidates(glm::min(point,end),glm::max(point,end))){const auto& t=surfaces[i];if(std::find(eligible.begin(),eligible.end(),t.objectId)==eligible.end())continue;float fraction;if(intersect(point,end,t,fraction)){auto it=first.find(t.objectId);if(it==first.end()||fraction<it->second.first)first[t.objectId]={fraction,glm::dot(end-point,t.normal)};}}
    for(const auto& hit:first)if(hit.second.second>0)return true;
    return false;
}
bool SpatialWorld::actorClear(glm::vec3 feet,float radius,float height) const {
    if(insideSolid(feet+glm::vec3(0,height*.5f,0)))return false;
    for(size_t i:candidates(feet-glm::vec3(radius,0,radius),feet+glm::vec3(radius,height,radius))) {
        const auto& tri=surfaces[i];glm::vec3 p,q;float distance=capsuleDistance(feet+glm::vec3(0,radius,0),feet+glm::vec3(0,height-radius,0),tri,p,q);
        if(distance<radius-.01f && !supportingFace(tri,feet,q))return false;
    }return true;
}
ActorMove SpatialWorld::moveActor(glm::vec3 feet,glm::vec3 movement,float radius,float height,bool grounded,float stepHeight) const {
    ActorMove result{feet,grounded,false};int steps=std::max(1,int(std::ceil(glm::length(movement)/std::max(.04f,radius*.4f))));steps=std::min(steps,512);auto delta=movement/float(steps);
    for(int step=0;step<steps;++step) {
        auto previous=result.position,next=previous+delta;bool floor=false;
        if(delta.y<=.0001f) {
            float above=result.grounded?stepHeight:std::max(0.f,previous.y-next.y)+skin;
            auto ground=support(next,above,result.grounded?stepHeight+.08f:skin,radius);
            if(ground.hit && (result.grounded || next.y<=ground.height+skin)){next.y=ground.height+skin;floor=true;}
        }
        for(int iteration=0;iteration<5;++iteration) {
            bool resolved=false;
            for(size_t i:candidates(next-glm::vec3(radius,0,radius),next+glm::vec3(radius,height,radius))) {
                const auto& tri=surfaces[i];glm::vec3 p,q;float distance=capsuleDistance(next+glm::vec3(0,radius,0),next+glm::vec3(0,height-radius,0),tri,p,q);
                if(distance>=radius-skin*.1f || supportingFace(tri,next,q))continue;
                auto normal=distance>1e-6f?(p-q)/distance:tri.normal;
                if(glm::dot(normal,(previous+glm::vec3(0,height*.5f,0))-q)<0)normal=-normal;
                next+=normal*(radius-distance+skin);resolved=true;if(normal.y<-.5f)result.ceiling=true;
            }
            if(!resolved)break;
        }
        if(delta.y<=0){auto ground=support(next,.04f,.09f,radius);floor=ground.hit && std::abs(next.y-ground.height)<.095f;}
        result.position=next;result.grounded=floor;
    }return result;
}
bool SpatialWorld::isWalkable(glm::vec3 feet,float radius,float height) const {
    auto ground=support(feet,.3f,.45f,radius);return ground.hit && std::abs(ground.height-feet.y)<=.45f && actorClear({feet.x,ground.height+skin,feet.z},radius,height);
}
bool SpatialWorld::canTraverse(glm::vec3 from,glm::vec3 to,float radius,float height) const {
    auto delta=to-from;float horizontal=glm::length(glm::vec2(delta.x,delta.z));
    if(std::abs(delta.y)>horizontal*1.0f+.1f)return false;
    if(raycast(from+glm::vec3(0,height*.55f,0),to+glm::vec3(0,height*.55f,0),radius).hit)return false;
    int steps=std::max(1,int(std::ceil(glm::length(delta)/.25f)));
    for(int i=0;i<=steps;++i) {
        auto p=glm::mix(from,to,float(i)/steps);if(!isWalkable(p,radius,height))return false;

    }return true;
}
glm::vec3 SpatialWorld::nextWaypoint(glm::vec3 from,glm::vec3 target,float radius) const {
    if(!isWalkable(from,radius)) {
        float best=std::numeric_limits<float>::infinity();glm::vec3 escape=from;
        for(float distance:{.06f,.12f,.24f})for(int direction=0;direction<16;++direction){float angle=direction*6.2831853f/16;auto candidate=from+glm::vec3(std::cos(angle)*distance,0,std::sin(angle)*distance);
            if(isWalkable(candidate,radius) && canTraverse(from,candidate,std::max(0.f,radius-.05f))){float cost=distance+glm::length(candidate-target)*.001f;if(cost<best){best=cost;escape=candidate;}}}
        if(escape!=from)return escape;
    }
    // A jumping target is projected downward onto its actual support layer.
    auto landing=support(target,.05f,3.f);
    if(landing.hit && target.y>landing.height+.1f)target.y=landing.height+.015f;
    if(canTraverse(from,target,radius))return target;
    const size_t count=navNodes.size();if(!count)return from;
    auto nearest=[&](glm::vec3 p) {std::vector<size_t> list(count);std::iota(list.begin(),list.end(),0);std::sort(list.begin(),list.end(),[&](size_t a,size_t b){return glm::length(navNodes[a].position-p)<glm::length(navNodes[b].position-p);});return list;};
    const float infinity=std::numeric_limits<float>::infinity();
    // All chasers share the same moving target. Reuse reverse graph distances;
    // per-actor attachment checks still test its actual supported, clear segment.
    if(!targetCached || glm::length(target-cachedTarget)>.35f || radius!=cachedRadius) {
        targetCached=true;cachedTarget=target;cachedRadius=radius;targetCosts.assign(count,infinity);targetNext.assign(count,-1);
        auto candidates=nearest(target);size_t accepted=0;
        for(size_t index:candidates) {
            if(accepted>=4)break;
            if(canTraverse(navNodes[index].position,target,radius)){targetCosts[index]=glm::length(navNodes[index].position-target);++accepted;}
        }
        std::vector<bool> used(count,false);
        for(size_t pass=0;pass<count;++pass){size_t current=count;for(size_t i=0;i<count;++i)if(!used[i]&&(current==count||targetCosts[i]<targetCosts[current]))current=i;
            if(current==count||!std::isfinite(targetCosts[current]))break;used[current]=true;
            for(size_t i=0;i<count;++i)if(targetCosts[current]+edges[i][current]<targetCosts[i]){targetCosts[i]=targetCosts[current]+edges[i][current];targetNext[i]=int(current);}
        }
    }
    float best=infinity;int destination=-1;auto candidates=nearest(from);size_t accepted=0;
    for(size_t index:candidates) {
        if(accepted>=4)break;
        if(!std::isfinite(targetCosts[index]))continue;
        float distance=glm::length(from-navNodes[index].position);
        if(canTraverse(from,navNodes[index].position,radius)){++accepted;float cost=distance+targetCosts[index];if(cost<best){best=cost;destination=distance<.35f?targetNext[index]:int(index);}}
    }
    if(destination<0)return from;
    return navNodes[size_t(destination)].position;
}
bool SpatialWorld::navigationConnected() const {
    if(navNodes.empty())return false;
    for(size_t origin=0;origin<navNodes.size();++origin){std::vector<bool> seen(navNodes.size(),false);std::vector<size_t> todo{origin};seen[origin]=true;
        for(size_t n=0;n<todo.size();++n)for(size_t i=0;i<navNodes.size();++i)if(!seen[i]&&std::isfinite(edges[todo[n]][i])){seen[i]=true;todo.push_back(i);}
        if(std::find(seen.begin(),seen.end(),false)!=seen.end())return false;
    }return true;
}
std::vector<std::string> SpatialWorld::diagnostics() const {
    std::vector<std::string> out;if(!navigationConnected())out.push_back("3D navigation is disconnected");
    for(const auto& node:navNodes)if(!isWalkable(node.position,.65f))out.push_back("Navigation node "+std::to_string(node.id)+" lacks support or clearance");
    for(const auto& link:navLinks) {
        auto a=std::find_if(navNodes.begin(),navNodes.end(),[&](const auto& n){return n.id==link.from;});auto b=std::find_if(navNodes.begin(),navNodes.end(),[&](const auto& n){return n.id==link.to;});
        if(a!=navNodes.end()&&b!=navNodes.end()&&!canTraverse(a->position,b->position,.65f))out.push_back("Navigation link "+std::to_string(link.from)+" -> "+std::to_string(link.to)+" crosses blocked or unsupported space");
    }return out;
}
}
