#include "character/crystal_creatures.h"
#include "character/character_presentation.h"
#include "enemy/crystal_combat.h"
#include "engine/shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <initializer_list>

namespace silic2 {
namespace {
using Vec = glm::vec3;
using Mat = glm::mat4;
constexpr float Pi = 3.141592654f;
const Vec Shell{.79f,.73f,.68f}, ShellSide{.57f,.51f,.53f};
const Vec Root{.25f,.19f,.23f}, Sinew{.37f,.26f,.32f};
const Vec Crystal{.53f,.36f,.76f}, CrystalDark{.33f,.22f,.53f}, Amber{.45f,.24f,.11f};
const Vec X{1,0,0}, Y{0,1,0}, Z{0,0,1};

struct Vertex { Vec position, normal, color; float joint, channel; };
struct Profile { Vec center; float radiusA, radiusB; };

class Sculpt {
public:
    std::vector<Vertex> mesh;
    int joint=0, channel=0;
    void face(Vec a, Vec b, Vec c, Vec outside, Vec color) {
        Vec n=glm::cross(b-a,c-a);
        const float len=glm::length(n);
        if(len<1.e-7f) return;
        if(glm::dot(n,outside)<0) {std::swap(b,c); n=-n;}
        n/=len;
        for(Vec p:{a,b,c}) mesh.push_back({p,n,color,float(joint),float(channel)});
    }

    // Connected changing cross-sections author paws, ribs, brows, armor plates
    // and folded roots. Only the face generation is shared between species.
    void loft(std::initializer_list<Profile> sections, Vec axisA, Vec axisB, int sides, Vec color,
              float twist=0.f, bool cap=true) {
        std::vector<Vec> prev(sides),next(sides);
        Vec previous{};
        int ring=0;
        for(auto section:sections) {
            for(int i=0;i<sides;++i) {
                const float angle=2.f*Pi*i/sides+twist;
                const float fault=1.f+.035f*std::sin(3.f*i+ring*.8f);
                next[i]=section.center+fault*(axisA*(std::cos(angle)*section.radiusA)
                                      +axisB*(std::sin(angle)*section.radiusB));
            }
            if(ring) for(int i=0;i<sides;++i) {
                int j=(i+1)%sides;
                Vec outside=(prev[i]+prev[j]+next[i]+next[j])*.25f-(previous+section.center)*.5f;
                Vec tint=color*(.90f+.10f*std::cos(i*2.4f+ring*.8f));
                face(prev[i],prev[j],next[j],outside,tint);
                face(prev[i],next[j],next[i],outside,tint*1.025f);
                if(ring==1 && cap) face(previous,prev[j],prev[i],previous-section.center,color*.85f);
                if(ring+1==int(sections.size()) && cap)
                    face(section.center,next[i],next[j],section.center-previous,color);
            }
            std::swap(prev,next); previous=section.center; ++ring;
        }
    }
    void z(std::initializer_list<Profile> sections,Vec color,int sides=7) {
        loft(sections,X,Y,sides,color,Pi*.5f);
    }
    void limb(Vec a,Vec b,float thick,float width,Vec color,int sides=6) {
        const Vec axis=glm::normalize(b-a);
        const Vec ref=std::abs(axis.x)<.85f ? X : Z;
        const Vec side=glm::normalize(glm::cross(axis,ref));
        const Vec across=glm::normalize(glm::cross(side,axis));
        loft({{a,thick*.72f,width*.65f},{glm::mix(a,b,.31f),thick,width},
              {glm::mix(a,b,.83f),thick*.66f,width*.73f},{b,thick*.35f,width*.5f}},
             side,across,sides,color);
    }
};

// A broken mineral plate has two uneven edges and a raised ridge. Wide front
// facets survive at swarm distances; the dark underside separates overlaps.
void shard(Sculpt& s,Vec heel,Vec tip,Vec across,float width,float thick,Vec color) {
    Vec axis=glm::normalize(tip-heel);
    across=glm::normalize(across-axis*glm::dot(across,axis));
    Vec ridge=glm::normalize(glm::cross(across,axis));
    s.loft({{heel,width*.42f,thick*.44f},
            {glm::mix(heel,tip,.29f),width,thick},
            {glm::mix(heel,tip,.67f)+across*.10f*width,width*.69f,thick*.59f},
            {tip,width*.055f,thick*.08f}},across,ridge,4,color);
}

// The Chaser is a tall split chisel. Its two mask plates expose a crushed
// geode inside; interlocking shins step beneath the overhanging mineral face.
void buildShardChaser(Sculpt& s) {
    s.joint=0; s.channel=0;
    s.loft({{{0,.52f,-.035f},.09f,.095f},{{0,.79f,-.065f},.14f,.14f},
            {{0,1.12f,-.04f},.20f,.17f},{{-.025f,1.35f,.015f},.15f,.14f},
            {{-.018f,1.48f,.045f},.10f,.085f}},X,Z,6,Root,.23f);
    for(int i=0;i<3;++i) {
        const float y=.70f+i*.14f;
        shard(s,{-.105f,y+.09f,.022f},{.12f,y-.055f,.125f},
              {0,1,0},.062f,.065f,i==1?Shell:ShellSide);
    }
    for(int sign:{-1,1}) {
        const float k=float(sign);
        shard(s,{k*.13f,1.10f,-.07f},{k*.26f,1.42f,-.17f},X,.087f,.09f,CrystalDark);
        shard(s,{k*.16f,1.16f,-.10f},{k*.22f,1.50f,-.22f},X,.051f,.055f,Crystal);
        // Broad shoulders end in points swept down the outside of the ribs.
        shard(s,{k*.13f,1.26f,-.09f},{k*.245f,.86f,.04f},
              {0,0,1},.142f,.075f,ShellSide);
    }
    s.loft({{{0,1.15f,.07f},.10f,.095f},{{-.013f,1.36f,.095f},.172f,.11f},
            {{-.032f,1.56f,.046f},.102f,.075f}},X,Z,6,CrystalDark,.10f);
    for(int sign:{-1,1}) {
        const float k=float(sign);
        const float crown=sign<0?1.77f:1.63f;
        // Each cheek flares to its own crown, then bites back into the seam.
        s.loft({{{k*.034f,1.13f,.19f},.016f,.022f},
                {{k*.089f,1.35f,.147f},.079f,.085f},
                {{k*.109f,1.48f,.104f},.10f,.097f},
                {{k*.115f,crown-.094f,-.015f},.047f,.052f},
                {{k*.145f,crown,-.064f},.007f,.012f}},
               X,Z,5,sign<0?Shell:Shell*.91f,.32f);
    }
    s.channel=1;
    shard(s,{0,1.34f,.205f},{-.014f,1.47f,.184f},X,.020f,.018f,Amber);
    s.channel=0;
    for(int sign:{-1,1}) {
        const float k=float(sign);
        s.joint=sign<0?1:2;
        const Vec hip{k*.106f,.66f,-.046f}, knee{k*.18f,.34f,.073f};
        const Vec ankle{k*.141f,.115f,-.066f};
        s.limb(hip,knee,.083f,.072f,Sinew,5);
        shard(s,hip+Vec{0,.014f,.012f},knee+Vec{k*.007f,-.064f,.025f},X,.077f,.084f,ShellSide);
        s.limb(knee,ankle,.046f,.042f,CrystalDark,5);
        shard(s,knee,ankle-Vec{0,.053f,0},X,.050f,.054f,Shell);
        shard(s,ankle,{k*.17f,.021f,.14f},X,.049f,.031f,ShellSide);
    }
}

// A low, fixed geode artillery rosette. The anchored tripod and concentric
// broken facets stay readable from above. Two shutters pull away from the
// amber firing chamber as the Sentry tracks and locks on.
void buildGeodeSentry(Sculpt& s) {
    s.joint=0; s.channel=0;
    for(int i=0;i<3;++i) {
        const float a=Pi*(.5f+i*2.f/3);
        const Vec out{std::cos(a),0,std::sin(a)}, sideways{out.z,0,-out.x};
        const Vec heel=Vec{0,.29f,-.035f}+out*.13f;
        const Vec joint=out*.31f+Vec{0,.16f,-.02f};
        const Vec foot=out*.52f+Vec{0,.029f,-.03f};
        s.limb(heel,joint,.14f,.115f,Sinew,5);
        shard(s,joint+Y*.075f,foot,sideways,.123f,.085f,ShellSide);
    }
    s.loft({{{0,.12f,-.045f},.16f,.14f},{{0,.28f,-.055f},.31f,.25f},
            {{0,.46f,-.085f},.42f,.28f},{{0,.69f,-.11f},.36f,.26f},
            {{0,.82f,-.13f},.24f,.17f}},X,Z,7,CrystalDark,.28f);
    for(int i=0;i<5;++i) {
        const float a=Pi*(1.08f+i*.21f);
        const Vec out{std::cos(a),0,std::sin(a)};
        const Vec tangent{out.z,0,-out.x};
        shard(s,out*.31f+Vec{0,.42f,-.035f},out*.46f+Vec{0,.82f+.083f*(i%2),-.09f},
              tangent,.135f,.105f,i%2?ShellSide:Shell);
    }
    // A real, forward-facing bore. The warm face sits behind the lids and
    // directly in front of its dark socket at the gameplay muzzle height.
    s.z({{{0,.60f,-.11f},.23f,.23f},{{0,.65f,.08f},.258f,.25f},
         {{0,.65f,.19f},.194f,.16f}},Root,8);
    s.channel=1;
    s.z({{{0,.65f,.15f},.135f,.118f},{{0,.65f,.20f},.133f,.12f},
         {{0,.65f,.245f},.063f,.051f}},Amber,7);
    s.channel=0;
    for(int sign:{-1,1}) {
        const float k=float(sign);
        s.joint=sign<0?1:2;
        s.loft({{{k*.34f,.41f,.032f},.079f,.088f},
                {{k*.24f,.54f,.139f},.168f,.141f},
                {{k*.15f,.72f,.18f},.145f,.14f},
                {{k*.106f,.84f,.126f},.080f,.09f},
                {{k*.071f,.912f,.06f},.010f,.02f}},X,Z,5,Shell,.26f);
        shard(s,{k*.205f,.45f,.088f},{k*.39f,.71f,.08f},X,.057f,.092f,Crystal);
    }
    s.joint=0;
    shard(s,{0,.36f,.16f},{0,.51f,.31f},X,.15f,.085f,ShellSide);
}

enum HoundPart { Trunk,Head,UpperJaw,LowerJaw,FirstLeg };
Vec hip(int leg) {
    float side=leg%2==0?1.f:-1.f;
    return leg<2?Vec(side*.31f,.87f,.28f):Vec(side*.20f,.57f,-.78f);
}
Vec knee(int leg) {
    float side=leg%2==0?1.f:-1.f;
    return leg<2?Vec(side*.44f,.55f,.02f):Vec(side*.27f,.39f,-.66f);
}
Vec foot(int leg) {
    float side=leg%2==0?1.f:-1.f;
    return leg<2?Vec(side*.42f,.095f,.48f):Vec(side*.29f,.075f,-1.12f);
}
void buildHound(Sculpt& s) {
    s.joint=Trunk;
    s.z({{{0,.52f,-1.24f},.09f,.08f},{{-.018f,.57f,-.93f},.23f,.21f},
         {{.012f,.64f,-.62f},.15f,.16f},{{0,.76f,-.39f},.18f,.20f},
         {{-.025f,.88f,.02f},.35f,.34f},{{0,.88f,.36f},.39f,.34f},
         {{0,.84f,.57f},.21f,.20f}},Root,9);
    // Shingled central ridge, narrow waist and recessed purple rib muscles.
    for(int i=0;i<5;++i) {
        float z=.44f-i*.32f, height=1.08f-std::max(0,i-1)*.105f;
        float w=.36f-std::max(0,i-1)*.047f;
        s.z({{{0,height-.12f,z-.40f},.02f,.025f},{{-.025f,height,z-.19f},w,.14f},
             {{.012f,height+.02f,z+.04f},w*.88f,.16f},{{0,height,z+.13f},w*.46f,.065f}},
             i%2?ShellSide:Shell);
    }
    for(int side:{-1,1}) for(int i=0;i<3;++i) {
        float z=.08f-i*.17f;
        s.limb({side*.28f,1.04f,z},{side*(.24f-.018f*i),.59f,z-.12f},
               .073f,.08f,i%2?CrystalDark:Crystal);
    }
    s.joint=Head;
    s.z({{{0,.97f,.30f},.23f,.21f},{{0,.91f,.56f},.26f,.24f},
         {{0,.89f,.81f},.19f,.14f}},Sinew);
    for(int side:{-1,1})
        s.limb({side*.20f,.96f,.45f},{side*.16f,.99f,.90f},.10f,.105f,ShellSide);
    s.channel=1;
    s.z({{{0,.88f,.65f},.115f,.075f},{{0,.88f,.79f},.135f,.10f},
         {{0,.87f,.87f},.06f,.055f}},Amber);
    s.channel=0;
    for(int lower=0;lower<2;++lower) {
        s.joint=lower?LowerJaw:UpperJaw;
        float y=lower?.76f:1.02f;
        s.z({{{0,y,.59f},.20f,.08f},{{0,y,.79f},.21f,.10f},
             {{0,y,.98f},.14f,.055f}},lower?ShellSide:Shell);
        for(int side:{-1,1}) {
            s.z({{{side*.112f,y,.81f},.095f,.065f},{{side*.12f,y,1.01f},.09f,.092f},
                 {{side*.15f,y+(lower?.04f:-.04f),1.21f},.055f,.058f},
                 {{side*.17f,y+(lower?.10f:-.075f),1.40f},.008f,.008f}},Shell,5);
        }
    }
    s.joint=Head;
    for(int side:{-1,1}) {
        // Joined cheek plates and a short dark hinge give the open jaws a
        // root to push against; the amber throat remains visible between them.
        s.z({{{side*.16f,.94f,.46f},.09f,.14f},{{side*.194f,.96f,.64f},.095f,.15f},
             {{side*.17f,1.00f,.80f},.043f,.06f}},ShellSide,6);
        s.limb({side*.19f,.87f,.54f},{side*.15f,.78f,.78f},.055f,.055f,Sinew,5);
        s.joint=UpperJaw;
        for(int i=0;i<2;++i) {
            float z=.87f+i*.15f;
            s.limb({side*.118f,.98f,z},{side*.12f,.88f,z+.036f},.037f,.034f,ShellSide,5);
        }
        s.joint=Head;
    }
    for(int leg=0;leg<4;++leg) {
        Vec a=hip(leg), b=knee(leg), c=foot(leg);
        const float w=leg<2?.16f:.105f;
        s.joint=FirstLeg+leg*2;
        s.limb(a,b,w*1.33f,w,Root);
        s.limb(a+Vec{0,.035f,0},glm::mix(a,b,.84f)+Vec{0,0,.055f},w*1.38f,w,Shell);
        s.joint++;
        s.limb(b,c,w,w*.74f,Sinew);
        s.limb(glm::mix(b,c,.16f)+Vec{0,0,.035f},glm::mix(b,c,.8f)+Vec{0,0,.06f},w,w*.83f,ShellSide);
        for(int toe:{-1,1})
            s.limb(c+Vec{toe*w*.35f,0,0},c+Vec{toe*w*.49f,-.035f,.16f},w*.45f,w*.53f,Shell);
    }
}

enum RayPart { Spine, LeftInner, LeftOuter, RightInner, RightOuter, Tail };
void buildRay(Sculpt& s) {
    s.joint=Spine;
    s.z({{{0,.32f,-.64f},.06f,.05f},{{0,.31f,-.41f},.17f,.14f},
         {{0,.32f,.09f},.28f,.23f},{{0,.35f,.36f},.20f,.19f},
         {{0,.4f,.82f},.012f,.025f}},Root);
    s.z({{{0,.45f,-.46f},.022f,.035f},{{0,.46f,-.15f},.13f,.15f},
         {{0,.52f,.24f},.16f,.13f},{{0,.36f,.82f},.02f,.03f}},Shell);
    s.z({{{0,.49f,-.48f},.02f,.01f},{{0,.52f,-.35f},.08f,.08f},
         {{0,.55f,-.17f},.09f,.105f},{{0,.57f,-.1f},.01f,.026f}},Crystal);
    s.channel=1;
    s.z({{{0,.11f,-.19f},.035f,.03f},{{0,.09f,.12f},.15f,.06f},
         {{0,.13f,.31f},.06f,.045f}},Amber);
    s.channel=0;
    for(int sign:{-1,1}) {
        const int inner=sign<0?LeftInner:RightInner;
        // Each wing has individually cut, double-faced mineral panels and a pale
        // leading spar. The elbow seam is an authored pivot, not a skin joint.
        Vec front[5]={{sign*.13f,.39f,.48f},{sign*.51f,.40f,.37f},
                      {sign*.85f,.33f,.24f},{sign*1.30f,.25f,-.04f},{sign*1.75f,.12f,-.38f}};
        Vec back[5]={{sign*.13f,.23f,-.42f},{sign*.48f,.14f,-.60f},
                     {sign*.85f,.18f,-.51f},{sign*1.19f,.17f,-.32f},front[4]};
        for(int i=0;i<4;++i) {
            s.joint=i<2?inner:inner+1;
            Vec a=front[i], b=front[i+1], c=back[i+1], d=back[i];
            Vec mid=(a+b+c+d)*.25f+Vec{0,.06f,0};
            for(int face=0;face<2;++face) {
                Vec n=face==0?Y:-Y;
                Vec ridge=face==0?mid:mid-Vec{0,.12f,0};
                Vec base=(i%2?CrystalDark:Crystal)*(face?.84f:1.f);
                s.face(a,b,ridge,n,base*.78f); s.face(b,c,ridge,n,base);
                s.face(c,d,ridge,n,base*.77f); s.face(d,a,ridge,n,base*1.12f);
            }
            s.limb(a,b,.063f*(1-i*.13f),.075f*(1-i*.15f),Shell);
            s.limb(a,d,.037f,.037f,Root,5);
        }
    }
    s.joint=Tail;
    s.z({{{0,.18f,-1.05f},.008f,.02f},{{.01f,.23f,-.84f},.055f,.07f},
         {{0,.27f,-.67f},.065f,.065f},{{0,.31f,-.44f},.08f,.09f}},ShellSide,6);
}

enum CenserPart { CenserBody, Crown, CenserTail, FirstPetal, Core=6 };
constexpr Vec mouth=crystalCombat::CenserMouth;
const Vec mouthNormal=glm::normalize(Vec{0,-.40f,.92f});
const Vec mouthUp=glm::normalize(glm::cross(mouthNormal,X));
Vec radial(float a) { return std::cos(a)*X+std::sin(a)*mouthUp; }
Vec rim(int petal) { return radial(2.f*Pi*petal/3.f+Pi*.5f); }
void buildCenser(Sculpt& s) {
    s.joint=CenserBody;
    // Lip opens forward and down. Half of the lower shell is cut away at its
    // front so the sac can actually be shot from the ground.
    s.loft({{{0,.70f,-.21f},.24f,.25f},{{0,1.00f,-.21f},.55f,.36f},
            {{0,1.45f,-.06f},.73f,.54f},{{-.02f,1.80f,-.04f},.73f,.55f},
            {{.035f,2.04f,-.03f},.48f,.34f}},X,Z,9,CrystalDark);
    for(int i=0;i<9;++i) {
        const float a=i*2*Pi/9.f+.12f;
        const Vec out{std::cos(a),0,std::sin(a)};
        float y=.70f+.32f*std::max(0.f,out.z);
        s.limb(out*.68f+Vec{0,1.72f,-.04f},out*.49f+Vec{0,1.14f,-.07f},.10f,.095f,Root,5);
        if(out.z<.3f)
            s.limb(out*.49f+Vec{0,1.14f,-.07f},out*.20f+Vec{0,y,-.15f},.11f,.085f,Sinew,5);
    }
    s.joint=Crown;
    s.loft({{{0,1.52f,-.04f},.68f,.50f},{{.018f,1.75f,-.025f},.78f,.61f},
            {{0,1.94f,-.015f},.66f,.52f},{{.018f,2.13f,-.035f},.41f,.32f},
            {{.008f,2.23f,-.045f},.24f,.21f}},X,Z,9,ShellSide,.18f);
    for(int i=0;i<8;++i) {
        const float a=i*2*Pi/8, t=i%3*.06f;
        Vec out{std::cos(a),0,std::sin(a)};
        Vec tangent{out.z,0,-out.x};
        s.loft({{out*.72f+Vec{0,1.47f+t,-.05f},.035f,.02f},
                {out*.71f+Vec{0,1.72f+t,-.03f},.18f,.085f},
                {out*.57f+Vec{0,1.93f+t,-.02f},.205f,.075f},
                {out*.22f+Vec{0,2.19f+t,-.035f},.055f,.03f}},
               tangent,out,5,Shell);
    }
    s.limb({-.06f,2.07f,-.03f},{.025f,2.37f,-.075f},.12f,.11f,Shell);
    s.joint=CenserTail;
    s.loft({{{.01f,.025f,-.16f},.02f,.018f},{{-.10f,.21f,-.18f},.085f,.08f},
            {{.05f,.40f,-.15f},.11f,.10f},{{-.07f,.58f,-.18f},.15f,.13f},
            {{0,.82f,-.18f},.22f,.2f}},X,Z,6,Sinew,.3f);
    s.joint=Core;
    s.loft({{mouth-mouthNormal*.14f,.18f,.17f},{mouth,.30f,.29f},
            {mouth+mouthNormal*.09f,.24f,.21f},{mouth+mouthNormal*.14f,.08f,.08f}},
           X,mouthUp,7,Amber);
    for(int i=0;i<3;++i) {
        s.joint=Core;s.channel=2+i;
        Vec top=mouth+mouthNormal*.14f+X*((i-1)*.135f);
        s.limb(top-mouthUp*.093f,top+mouthUp*.10f,.039f,.032f,Amber);
        s.channel=0;
        s.joint=FirstPetal+i;
        const Vec out=rim(i),axis=glm::cross(mouthNormal,out);
        const Vec pivot=mouth+out*.41f;
        s.loft({{pivot+out*.045f-mouthNormal*.035f,.14f,.06f},
                {pivot-out*.05f+mouthNormal*.08f,.31f,.075f},
                {pivot-out*.27f+mouthNormal*.18f,.19f,.08f},
                {mouth+mouthNormal*.23f,.022f,.025f}},axis,mouthNormal,5,Shell);
    }
}

using Pose = std::array<Mat, 32>;
Mat hinge(Vec origin, float radians, Vec axis) {
    return glm::translate(glm::rotate(glm::translate(Mat(1),origin),radians,axis),-origin);
}
float smooth(float t) { t=std::clamp(t,0.f,1.f); return t*t*(3-2*t); }
bool phase(const CharacterView& c,AttackPhase p) { return c.attackPhase==p; }

void poseHound(const CharacterView& c,float time,Pose& m) {
    const float p=std::clamp(c.attackProgress,0.f,1.f);
    const float ready=phase(c,AttackPhase::Windup)?smooth(p*3.f):0.f;
    const float run=phase(c,AttackPhase::Attack)?1.f:0.f;
    const float braking=phase(c,AttackPhase::Recovery)?1.f-smooth(p):0.f;
    const float moving=phase(c,AttackPhase::Approach)?std::clamp(c.movement,0.f,1.f):0.f;
    const float swing=std::sin(time*(8.f+5.f*moving)+c.phase)*moving;
    const float crouch=.20f*ready+.085f*run;
    m[Trunk]=glm::translate(Mat(1),Vec{0,-crouch+.025f*std::abs(swing),.075f*run})
             *hinge({0,.75f,0},.095f*ready-.12f*braking,X);
    m[Head]=m[Trunk]*hinge({0,.92f,.32f},.09f*ready-.075f*braking,X);
    const float gape=.07f+.61f*ready+.47f*run+.18f*braking;
    m[UpperJaw]=m[Head]*hinge({0,.93f,.59f},-gape*.35f,X);
    m[LowerJaw]=m[Head]*hinge({0,.84f,.59f},gape*.65f,X);
    for(int leg=0;leg<4;++leg) {
        const Vec a=hip(leg),b=knee(leg),f=foot(leg);
        float step=(leg==0||leg==3 ? swing : -swing);
        Vec start=Vec(m[Trunk]*glm::vec4(a,1));
        Vec planted=f+Vec{0,.085f*std::max(0.f,step),.24f*step
                            +(leg<2?.23f:-.23f)*run+.16f*braking};
        // Solve each two-piece leg toward its planted toe. The knee bends
        // backwards on the heavy forelimbs and forwards on the small rear legs.
        const float upperLength=glm::length(b-a),lowerLength=glm::length(f-b);
        Vec line=planted-start;
        float length=std::clamp(glm::length(line),std::abs(upperLength-lowerLength)+.001f,
                                upperLength+lowerLength-.001f);
        Vec direction=glm::normalize(line);
        Vec preferred=leg<2?-Z:Z;
        Vec bend=glm::normalize(preferred-direction*glm::dot(preferred,direction));
        float along=(upperLength*upperLength-lowerLength*lowerLength+length*length)/(2.f*length);
        Vec middle=start+along*direction+bend*std::sqrt(std::max(0.f,upperLength*upperLength-along*along));
        Vec toe=start+direction*length;
        auto segment=[](Vec from,Vec to,Vec origin,Vec end) {
            const Vec u=glm::normalize(to-from),v=glm::normalize(end-origin);
            Vec axis=glm::cross(u,v);
            Mat rotation(1);
            if(glm::length(axis)>1.e-5f)
                rotation=glm::rotate(rotation,std::atan2(glm::length(axis),glm::dot(u,v)),glm::normalize(axis));
            return glm::translate(Mat(1),origin)*rotation*glm::translate(Mat(1),-from);
        };
        m[FirstLeg+leg*2]=segment(a,b,start,middle);
        m[FirstLeg+leg*2+1]=segment(b,f,middle,toe);
    }
}
void poseRay(const CharacterView& c,float time,Pose& m) {
    float p=std::clamp(c.attackProgress,0.f,1.f);
    const float beat=std::sin(time*2.4f+c.phase);
    float fold=phase(c,AttackPhase::Windup)?.40f*smooth(p*4.f):0;
    if(phase(c,AttackPhase::Attack)) fold=.38f*(1-p);
    const float bank=phase(c,AttackPhase::Recovery)?.37f*std::sin(p*Pi):
                     phase(c,AttackPhase::Approach)?.08f*beat:0;
    m[Spine]=hinge({0,.31f,0},bank,Z);
    for(int sign:{-1,1}) {
        const int part=sign<0?LeftInner:RightInner;
        m[part]=m[Spine]*hinge({sign*.13f,.36f,.05f},sign*(.055f*beat+fold),Z);
        m[part+1]=m[part]*hinge({sign*.85f,.25f,-.10f},sign*(.09f*beat+fold*.24f),Z);
    }
    m[Tail]=m[Spine]*hinge({0,.28f,-.45f},beat*.16f,Y);
}
void poseCenser(const CharacterView& c,float time,Pose& m) {
    float p=std::clamp(c.attackProgress,0.f,1.f);
    bool charging=phase(c,AttackPhase::Windup);
    // The exposed organ is stationary in actor coordinates, exactly at the
    // combat hit-region center. Buoyancy and interrupt drop move the actor root.
    const bool released=phase(c,AttackPhase::Recovery) && c.interruptHits<crystalCombat::GlandHitLimit;
    float recoil=released?std::sin(std::min(p*5.f,1.f)*Pi)*.12f:0;
    float breath=charging?smooth(p*4.f):(.04f+.04f*std::sin(time*2.f+c.phase));
    const float shock=phase(c,AttackPhase::Interrupted)?1-smooth(p):0;
    const Mat body=glm::translate(Mat(1),Vec{0,recoil,0})*hinge(mouth,.19f*shock,X);
    // The inner sac expands out to the rigid crown. The petals and exposed
    // gland keep their combat-space pivots while the walls swell behind them.
    m[CenserBody]=glm::scale(body,Vec{1.f+.075f*breath,1.f,1.f+.045f*breath});
    m[Crown]=body*glm::translate(Mat(1),Vec{0,breath*.085f-shock*.065f,0});
    m[CenserTail]=body*hinge({0,.74f,-.18f},.12f*std::sin(time*2.f+c.phase)+shock*.45f,Z);
    m[Core]=body;
    for(int i=0;i<3;++i) {
        Vec out=rim(i);
        // Open rapidly when the interrupt window starts; the petals remain
        // behind the hit region for the rest of the opening.
        float open=charging ? (1.33f+.20f*smooth(p*10.f)) : .025f;
        m[FirstPetal+i]=body*hinge(mouth+out*.41f,open,glm::normalize(glm::cross(out,mouthNormal)));
    }
}
} // namespace

CrystalCreatures::CrystalCreatures()=default;
CrystalCreatures::~CrystalCreatures() {
    if(vao) glDeleteVertexArrays(1,&vao);
    if(buffer) glDeleteBuffers(1,&buffer);
    if(instanceBuffer) glDeleteBuffers(1,&instanceBuffer);
}

bool CrystalCreatures::init() {
    shader=std::make_unique<Shader>("res/shaders/crystal_creature.vert","res/shaders/crystal_creature.frag");
    GLint linked=0;
    glGetProgramiv(shader->ID,GL_LINK_STATUS,&linked);
    if(!linked) return false;
    minionShader=std::make_unique<Shader>("res/shaders/crystal_minion.vert","res/shaders/crystal_creature.frag");
    glGetProgramiv(minionShader->ID,GL_LINK_STATUS,&linked);
    if(!linked) return false;
    Sculpt sculpt;
    void (*builders[])(Sculpt&)={buildHound,buildRay,buildCenser,buildShardChaser,buildGeodeSentry};
    for(size_t i=0;i<ranges.size();++i) {
        ranges[i].first=static_cast<int>(sculpt.mesh.size());
        builders[i](sculpt);
        ranges[i].count=static_cast<int>(sculpt.mesh.size())-ranges[i].first;
    }
    GLint prevVao=0,prevBuffer=0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&prevVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&prevBuffer);
    glGenVertexArrays(1,&vao); glGenBuffers(1,&buffer);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER,buffer);
    glBufferData(GL_ARRAY_BUFFER,sculpt.mesh.size()*sizeof(Vertex),sculpt.mesh.data(),GL_STATIC_DRAW);
    const size_t offsets[]={offsetof(Vertex,position),offsetof(Vertex,normal),offsetof(Vertex,color),
                            offsetof(Vertex,joint),offsetof(Vertex,channel)};
    for(unsigned i=0;i<5;++i) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i,i<3?3:1,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsets[i]));
    }
    glGenBuffers(1,&instanceBuffer);
    glBindBuffer(GL_ARRAY_BUFFER,instanceBuffer);
    glBufferData(GL_ARRAY_BUFFER,sizeof(MinionInstance),nullptr,GL_STREAM_DRAW);
    const size_t instanceOffsets[]={offsetof(MinionInstance,feetAndSinYaw),offsetof(MinionInstance,pose),offsetof(MinionInstance,hit)};
    for(unsigned i=0;i<3;++i) {
        glEnableVertexAttribArray(i+5);
        glVertexAttribPointer(i+5,i==2?1:4,GL_FLOAT,GL_FALSE,sizeof(MinionInstance),
                              reinterpret_cast<void*>(instanceOffsets[i]));
        glVertexAttribDivisor(i+5,1);
    }
    glBindVertexArray(prevVao); glBindBuffer(GL_ARRAY_BUFFER,prevBuffer);
    projectionLocation=glGetUniformLocation(shader->ID,"projectionView");
    rootLocation=glGetUniformLocation(shader->ID,"root");
    poseLocation=glGetUniformLocation(shader->ID,"pose[0]");
    attackLocation=glGetUniformLocation(shader->ID,"attackLight");
    hitLocation=glGetUniformLocation(shader->ID,"hit");
    marksLocation=glGetUniformLocation(shader->ID,"coreHits");
    minionProjection=glGetUniformLocation(minionShader->ID,"projectionView");
    minionSpecies=glGetUniformLocation(minionShader->ID,"species");
    return vao && buffer && instanceBuffer && poseLocation>=0 && minionSpecies>=0;
}

void CrystalCreatures::draw(const Mat& view,const Mat& projection,
                            const std::vector<CharacterView>& characters,float time) {
    for(auto& batch:minions) batch.clear();
    bool hasHeavy=false;
    for(const auto& c:characters) {
        if(crystalCombat::isCrystalEnemy(c.type)) {hasHeavy=true;continue;}
        if(!c.crystalMinion || (c.type!=EnemyType::Chaser && c.type!=EnemyType::Sentry)) continue;
        const auto heading=glm::vec2(c.heading.x,c.heading.z);
        const float length=glm::length(heading);
        const auto direction=length>.0001f ? heading/length : glm::vec2(0,1);
        const bool sentry=c.type==EnemyType::Sentry;
        const float charge=c.charging?std::clamp(c.chargeProgress,0.f,1.f):0.f;
        const float gait=sentry ? 0.f : std::sin(time*11.f+c.phase)*std::clamp(c.movement,0.f,1.f);
        const float open=charge*(c.aimLocked?1.f:.78f);
        const float light=sentry ? (c.charging?(c.aimLocked?1.f:.32f+.44f*charge):.05f) : .26f;
        minions[sentry?1:0].push_back({glm::vec4(c.position,direction.x),{direction.y,gait,open,light},
                                       std::clamp(c.hitFlash,0.f,1.f)});
    }
    if(!hasHeavy && minions[0].empty() && minions[1].empty()) return;
    GLint prevProgram,prevVao,prevDepthFunc;
    glGetIntegerv(GL_CURRENT_PROGRAM,&prevProgram); glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&prevVao);
    glGetIntegerv(GL_DEPTH_FUNC,&prevDepthFunc);
    GLboolean prevMask,depth=glIsEnabled(GL_DEPTH_TEST),blend=glIsEnabled(GL_BLEND),cull=glIsEnabled(GL_CULL_FACE);
    glGetBooleanv(GL_DEPTH_WRITEMASK,&prevMask);
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); glDepthMask(GL_TRUE);
    glDisable(GL_BLEND); glDisable(GL_CULL_FACE);
    glBindVertexArray(vao);
    const Mat vp=projection*view;
    if(hasHeavy) {
        shader->use();
        glUniformMatrix4fv(projectionLocation,1,GL_FALSE,glm::value_ptr(vp));
    }
    for(const auto& c:characters) {
        if(!crystalCombat::isCrystalEnemy(c.type)) continue;
        Pose pose;
        pose.fill(Mat(1));
        int species, boneCount;
        if(c.type==EnemyType::ForkHound) {species=0; boneCount=FirstLeg+8; poseHound(c,time,pose);}
        else if(c.type==EnemyType::VeilRay) {species=1; boneCount=Tail+1; poseRay(c,time,pose);}
        else {species=2; boneCount=Core+1; poseCenser(c,time,pose);}
        Mat root=glm::rotate(glm::translate(Mat(1),c.position),std::atan2(c.heading.x,c.heading.z),Y);
        float light=c.attackPhase==AttackPhase::Attack?1.f:
                    c.attackPhase==AttackPhase::Windup?.70f+.30f*std::clamp(c.attackProgress,0.f,1.f):0.f;
        // Pose pivots and part vertices share model space. Scale them together
        // about the actor origin, after constructing the world translation.
        root=glm::scale(root,Vec(crystalCombat::bodyScale(c.type)));
        glUniformMatrix4fv(rootLocation,1,GL_FALSE,glm::value_ptr(root));
        glUniformMatrix4fv(poseLocation,boneCount,GL_FALSE,glm::value_ptr(pose.front()));
        // Keep the three amber marks legible against a repeatedly hit shell.
        const bool showingCore=c.type==EnemyType::HangingCenser &&
            (c.attackPhase==AttackPhase::Windup || c.attackPhase==AttackPhase::Interrupted);
        glUniform1f(attackLocation,light);
        glUniform1f(hitLocation,std::clamp(c.hitFlash,0.f,1.f)*(showingCore?.22f:1.f));
        glUniform1i(marksLocation,c.interruptHits);
        glDrawArrays(GL_TRIANGLES,ranges[species].first,ranges[species].count);
    }
    if(!minions[0].empty() || !minions[1].empty()) {
        GLint prevBuffer;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&prevBuffer);
        glBindBuffer(GL_ARRAY_BUFFER,instanceBuffer);
        minionShader->use();
        glUniformMatrix4fv(minionProjection,1,GL_FALSE,glm::value_ptr(vp));
        for(int species=0;species<2;++species) {
            const auto& batch=minions[species];
            if(batch.empty()) continue;
            const auto& range=ranges[3+species];
            glBufferData(GL_ARRAY_BUFFER,batch.size()*sizeof(MinionInstance),batch.data(),GL_STREAM_DRAW);
            glUniform1i(minionSpecies,species);
            glDrawArraysInstanced(GL_TRIANGLES,range.first,range.count,static_cast<GLsizei>(batch.size()));
        }
        glBindBuffer(GL_ARRAY_BUFFER,prevBuffer);
    }
    glBindVertexArray(prevVao); glUseProgram(prevProgram);
    glDepthMask(prevMask); glDepthFunc(prevDepthFunc);
    if(!depth) glDisable(GL_DEPTH_TEST);
    if(blend) glEnable(GL_BLEND);
    if(cull) glEnable(GL_CULL_FACE);
}
}
