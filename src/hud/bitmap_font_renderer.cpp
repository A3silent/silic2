#include "hud/bitmap_font_renderer.h"
#include <algorithm>
#include <cmath>
#include <cctype>

namespace silic2 {

void HudBatch::triangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, const glm::vec4& color) {
    vertices.insert(vertices.end(), {{a,color},{b,color},{c,color}});
}
void HudBatch::rect(float x,float y,float w,float h,const glm::vec4& color) {
    if (w <= 0 || h <= 0 || color.a <= 0) return;
    triangle({x,y},{x+w,y},{x+w,y+h},color);
    triangle({x,y},{x+w,y+h},{x,y+h},color);
}
void HudBatch::line(glm::vec2 a,glm::vec2 b,float width,const glm::vec4& color) {
    const glm::vec2 delta=b-a;
    const float length=glm::length(delta);
    if (length <= 0 || width <= 0) return;
    const glm::vec2 n=glm::vec2(-delta.y,delta.x)*(width*0.5f/length);
    triangle(a+n,a-n,b-n,color); triangle(a+n,b-n,b+n,color);
}

std::array<uint8_t,7> BitmapFontRenderer::glyph(char character) {
    // Hand-authored row masks: bit 4 is the leftmost pixel. No font assets.
    switch (std::toupper(static_cast<unsigned char>(character))) {
    case 'A': return {14,17,17,31,17,17,17};
    case 'B': return {30,17,17,30,17,17,30};
    case 'C': return {14,17,16,16,16,17,14};
    case 'D': return {30,17,17,17,17,17,30};
    case 'E': return {31,16,16,30,16,16,31};
    case 'F': return {31,16,16,30,16,16,16};
    case 'G': return {14,17,16,23,17,17,15};
    case 'H': return {17,17,17,31,17,17,17};
    case 'I': return {31,4,4,4,4,4,31};
    case 'J': return {7,2,2,2,18,18,12};
    case 'K': return {17,18,20,24,20,18,17};
    case 'L': return {16,16,16,16,16,16,31};
    case 'M': return {17,27,21,21,17,17,17};
    case 'N': return {17,25,25,21,19,19,17};
    case 'O': return {14,17,17,17,17,17,14};
    case 'P': return {30,17,17,30,16,16,16};
    case 'Q': return {14,17,17,17,21,18,13};
    case 'R': return {30,17,17,30,20,18,17};
    case 'S': return {15,16,16,14,1,1,30};
    case 'T': return {31,4,4,4,4,4,4};
    case 'U': return {17,17,17,17,17,17,14};
    case 'V': return {17,17,17,17,17,10,4};
    case 'W': return {17,17,17,21,21,27,17};
    case 'X': return {17,17,10,4,10,17,17};
    case 'Y': return {17,17,10,4,4,4,4};
    case 'Z': return {31,1,2,4,8,16,31};
    case '0': return {14,17,19,21,25,17,14};
    case '1': return {4,12,4,4,4,4,14};
    case '2': return {14,17,1,2,4,8,31};
    case '3': return {30,1,1,14,1,1,30};
    case '4': return {2,6,10,18,31,2,2};
    case '5': return {31,16,16,30,1,1,30};
    case '6': return {14,16,16,30,17,17,14};
    case '7': return {31,1,2,4,8,8,8};
    case '8': return {14,17,17,14,17,17,14};
    case '9': return {14,17,17,15,1,1,14};
    case ':': return {0,4,4,0,4,4,0};
    case '.': return {0,0,0,0,0,6,6};
    case ',': return {0,0,0,0,6,4,8};
    case '-': return {0,0,0,31,0,0,0};
    case '_': return {0,0,0,0,0,0,31};
    case '/': return {1,2,2,4,8,8,16};
    case '%': return {25,26,2,4,8,11,19};
    case '+': return {0,4,4,31,4,4,0};
    case '[': return {14,8,8,8,8,8,14};
    case ']': return {14,2,2,2,2,2,14};
    case '!': return {4,4,4,4,4,0,4};
    case ' ': return {0,0,0,0,0,0,0};
    default: return {14,17,1,2,4,0,4};
    }
}
float BitmapFontRenderer::width(std::string_view text,float pixelSize) {
    size_t longest=0,current=0;
    for (char c:text) { if(c=='\n') { longest=std::max(longest,current);current=0; } else ++current; }
    longest=std::max(longest,current);
    return longest ? (static_cast<float>(longest)*6-1)*pixelSize : 0;
}
void BitmapFontRenderer::append(HudBatch& batch,std::string_view text,float x,float y,float p,const glm::vec4& color) {
    float start=x;
    for(char c:text) {
        if(c=='\n') { x=start;y+=9*p;continue; }
        auto rows=glyph(c);
        for(int row=0;row<7;++row) {
            // Merge adjacent set pixels into spans to reduce vertex traffic.
            for(int col=0;col<5;) {
                if(!(rows[row] & (1<<(4-col)))) { ++col;continue; }
                int first=col++;
                while(col<5 && (rows[row] & (1<<(4-col)))) ++col;
                batch.rect(x+first*p,y+row*p,(col-first)*p,p,color);
            }
        }
        x+=6*p;
    }
}

namespace {
const glm::vec4 ink(0.035f,0.055f,0.105f,0.94f);
const glm::vec4 white(0.91f,0.96f,1.0f,1.0f);
const glm::vec4 muted(0.55f,0.67f,0.76f,1.0f);
const glm::vec4 cyan(0.30f,0.94f,0.94f,1.0f);
const glm::vec4 warm(1.0f,0.48f,0.28f,1.0f);
float ratio(float hp,float maxHp) { return maxHp>0 ? std::clamp(hp/maxHp,0.0f,1.0f):0; }
std::string integer(float number) { return std::to_string(static_cast<int>(std::ceil(std::max(number,0.0f)))); }
float fade(float age,float lifetime) { return age>=0 && age<lifetime ? 1-age/lifetime:0; }
void text(HudBatch& b,std::string_view value,float x,float y,float p,const glm::vec4& color=white) {
    BitmapFontRenderer::append(b,value,std::round(x),std::round(y),p,color);
}
void centered(HudBatch& b,std::string_view value,float x,float y,float p,const glm::vec4& color=white) {
    text(b,value,x-BitmapFontRenderer::width(value,p)*0.5f,y,p,color);
}
void panel(HudBatch& b,float x,float y,float w,float h) {
    b.rect(x,y,w,h,ink); b.rect(x,y,2,h,cyan);
}
void minimap(HudBatch& b,const HudViewModel& m,float x,float y,float size,float p) {
    panel(b,x,y,size,size+25);
    const float inner=size-16;
    const glm::vec2 lo=m.mapMinXZ, span=glm::max(m.mapMaxXZ-lo,glm::vec2(1));
    auto point=[&](glm::vec2 world) { return glm::vec2(x+8,y+8)+glm::clamp((world-lo)/span,glm::vec2(0),glm::vec2(1))*inner; };
    auto inside=[&](glm::vec2 world) { return world.x>=lo.x && world.y>=lo.y && world.x<=lo.x+span.x && world.y<=lo.y+span.y; };
    b.rect(x+8,y+8,inner,inner,{0.06f,0.1f,0.16f,1});
    for(const auto& terrain:m.terrain) {
        const glm::vec2 a=point(terrain.minXZ),z=point(terrain.maxXZ);
        b.rect(a.x,a.y,z.x-a.x,z.y-a.y,{0.27f,0.35f,0.47f,1});
    }
    for(const auto& r:m.rifts) {
        if(!inside(r.worldXZ)) continue;
        const glm::vec2 q=point(r.worldXZ);
        const glm::vec4 color=r.warning ? warm:cyan;
        const float radius=r.warning ? 5.0f:3.0f;
        b.line(q+glm::vec2(0,-radius),q+glm::vec2(radius,0),1.5f,color);
        b.line(q+glm::vec2(radius,0),q+glm::vec2(0,radius),1.5f,color);
        b.line(q+glm::vec2(0,radius),q+glm::vec2(-radius,0),1.5f,color);
        b.line(q+glm::vec2(-radius,0),q+glm::vec2(0,-radius),1.5f,color);
    }
    for(const auto& e:m.enemies) {
        if(!hudEnemyVisible(e) || !inside(e.worldXZ)) continue;
        const glm::vec2 q=point(e.worldXZ);
        b.rect(q.x-2.5f,q.y-2.5f,5,5,warm);
        if(e.sentry) b.rect(q.x-1,q.y-1,2,2,ink);
    }
    const glm::vec2 q=point(m.playerXZ);
    glm::vec2 f=m.playerForwardXZ;
    f=glm::length(f)>0.001f ? glm::normalize(f):glm::vec2(0,-1);
    const glm::vec2 right(-f.y,f.x);
    b.triangle(q+f*6.0f,q-f*4.0f-right*4.0f,q-f*4.0f+right*4.0f,white);
    text(b,"N",x+size-16,y+10,p,muted);
    text(b,"VISIBLE CONTACTS",x+8,y+size+6,1,muted);
}
}

void buildHudGeometry(HudBatch& b,const HudViewModel& m,int logicalWidth,int logicalHeight) {
    b.vertices.clear();
    if(logicalWidth<=0 || logicalHeight<=0) return;
    // Design at at least 640x480; scale the complete layout down only for tiny windows.
    const float scale=std::min({1.0f,logicalWidth/640.0f,logicalHeight/480.0f});
    const float w=logicalWidth/scale,h=logicalHeight/scale,cx=w*0.5f;
    const float p=2.0f,margin=20;
    const bool live=m.phase==HudPhase::Playing || m.phase==HudPhase::Paused;
    if(live) {
        for(const auto& e:m.enemies) {
            if(!hudEnemyBarVisible(e)) continue;
            const float x=std::clamp(e.healthAnchor.x*w-25,2.0f,w-52),y=std::clamp(e.healthAnchor.y*h,2.0f,h-8);
            b.rect(x-1,y-1,52,7,ink);b.rect(x,y,50*ratio(e.hp,e.maxHp),5,warm);
        }
        panel(b,margin,margin,178,63);
        text(b,"COMBAT",margin+12,margin+10,1,muted);
        text(b,hudClock(m.combatSeconds),margin+12,margin+29,3);
        const std::string phase=hudPhaseLabel(m);
        centered(b,phase,cx,margin+6,p,cyan);
        centered(b,hudObjectiveLabel(m),cx,margin+29,1,muted);
        minimap(b,m,w-margin-156,margin,156,1);
        panel(b,margin,h-margin-67,218,67);
        text(b,"HP",margin+12,h-margin-55,p,muted);
        text(b,integer(m.hp)+" / "+integer(m.maxHp),margin+55,h-margin-55,p);
        b.rect(margin+12,h-margin-25,194,10,{0.13f,0.20f,0.27f,1});
        b.rect(margin+12,h-margin-25,194*ratio(m.hp,m.maxHp),10,m.hp<=m.maxHp*0.3f?warm:cyan);
        text(b,"ESC  PAUSE",w-margin-118,h-margin-14,1,muted);
        const float cy=h*0.5f;
        const glm::vec4 cross=fade(m.killAge,0.35f)>0 ? warm:white;
        for(int s:{-1,1}) {
            b.rect(cx+s*8-1,cy-1,2,2,ink);
            b.rect(cx-1,cy+s*8-1,2,2,ink);
            b.line({cx+s*5,cy},{cx+s*10,cy},3,ink);
            b.line({cx,cy+s*5},{cx,cy+s*10},3,ink);
            b.line({cx+s*5,cy},{cx+s*10,cy},1,cross);
            b.line({cx,cy+s*5},{cx,cy+s*10},1,cross);
        }
        const float hit=std::max(fade(m.hitAge,0.16f),fade(m.killAge,0.32f));
        if(hit>0) {
            for(int sx:{-1,1}) for(int sy:{-1,1})
                b.line({cx+sx*13,cy+sy*13},{cx+sx*8,cy+sy*8},2,{cross.r,cross.g,cross.b,hit});
        }
        if(fade(m.killAge,0.6f)>0) centered(b,"KILL",cx,cy+28,1,{warm.r,warm.g,warm.b,fade(m.killAge,0.6f)});
        const float hurt=fade(m.damageAge,0.8f);
        if(hurt>0) {
            glm::vec2 d(std::sin(m.damageDirectionRadians),-std::cos(m.damageDirectionRadians));
            glm::vec2 side(-d.y,d.x),center(cx,cy),tip=center+d*77.0f;
            b.triangle(tip,center+d*94.0f+side*9.0f,center+d*94.0f-side*9.0f,{1,0.34f,0.2f,hurt});
        }
    }
    if(m.phase!=HudPhase::Playing) {
        b.rect(0,0,w,h,{0.015f,0.025f,0.055f,live?0.72f:0.62f});
        const bool result=m.phase==HudPhase::Won || m.phase==HudPhase::Dead;
        const float pw=std::min(560.0f,w-48),ph=result?390.0f:m.phase==HudPhase::Ready?352.0f:220.0f;
        const float x=cx-pw*0.5f,y=(h-ph)*0.5f;
        panel(b,x,y,pw,ph);
        text(b,m.arenaSurvival?"SILIC2 / CRYSTAL ARENA":"SILIC2 / MAP",x+26,y+22,1,cyan);
        const std::string title=m.phase==HudPhase::Ready?"READY":m.phase==HudPhase::Paused?"PAUSED":m.phase==HudPhase::Won?"WIN":"DEAD";
        text(b,title,x+26,y+48,5,m.phase==HudPhase::Dead?warm:white);
        b.rect(x+26,y+102,pw-52,1,{0.2f,0.3f,0.4f,1});
        if(m.phase==HudPhase::Ready) {
            text(b,hudReadyObjective(m),x+26,y+123,p);
            text(b,"WASD     MOVE",x+26,y+161,p,muted);
            text(b,"MOUSE    AIM / LEFT CLICK FIRE",x+26,y+186,p,muted);
            text(b,"SHIFT    TOGGLE SPRINT",x+26,y+211,p,muted);
            text(b,"CTRL     CROUCH / SLIDE",x+26,y+236,p,muted);
            text(b,"SPACE    JUMP       ESC  PAUSE",x+26,y+261,p,muted);
            text(b,"ENTER  BEGIN",x+26,y+309,p,cyan);
        } else if(m.phase==HudPhase::Paused) {
            text(b,"COMBAT AND SOUND ARE FROZEN",x+26,y+124,p,muted);
            text(b,"ESC  RESUME",x+26,y+177,p,cyan);
        } else {
            const std::string labels[]={"TIME","KILLS","ACCURACY","HP LOST","FATAL SOURCE"};
            std::string fatal=m.phase==HudPhase::Won?"NONE":hudFatalSource(m.fatalSource);
            if(fatal.size()>20) fatal=fatal.substr(0,19)+".";
            const std::string values[]={hudClock(m.combatSeconds),std::to_string(m.kills),hudAccuracy(m.shotsHit,m.shotsFired),hudHpLoss(m.actualHpLoss),fatal};
            for(int i=0;i<5;++i) {
                text(b,labels[i],x+26,y+125+i*34,p,muted);
                text(b,values[i],x+pw-26-BitmapFontRenderer::width(values[i],p),y+125+i*34,p);
            }
            text(b,m.resultLockRemaining>0 ? "RESTART LOCKED - 1 SECOND":"R / LEFT CLICK  RESTART",x+26,y+327,p,m.resultLockRemaining>0?muted:cyan);
            text(b,"SAME SEED  "+std::to_string(m.seed),x+26,y+360,1,muted);
        }
    }
    if(m.audioStatus!=HudAudioStatus::Available) {
        const std::string label=m.audioStatus==HudAudioStatus::Muted ? "AUDIO MUTED":"AUDIO UNAVAILABLE";
        const float tw=BitmapFontRenderer::width(label,1);
        b.rect(cx-tw*0.5f-8,h-18,tw+16,16,ink);
        centered(b,label,cx,h-14,1,warm);
    }
    if(scale!=1) for(auto& v:b.vertices) v.position*=scale;
}
} // namespace silic2
