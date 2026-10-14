#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
out vec4 FragColor;
uniform vec3 objectColor;
uniform vec3 ambientLight;
uniform bool lightingEnabled;
uniform int materialKind; // 0 stone, 1 floor, 2 living crystal
uniform float phase;
uniform float time;
uniform vec4 riftWarnings; // Authored semantic slots; -1 means inactive.
uniform bool riftActive[4];
uniform mat4 riftWorldToLocal[4];
uniform vec2 floorCenter;
uniform vec2 floorHalfExtent;
uniform vec3 brushMinimum;
uniform vec3 brushSize;
uniform bool useTexture;
uniform sampler2D texture1;
struct Light { vec3 position; vec3 color; float intensity; float range; };
uniform Light lights[128];
uniform int numLights;
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float line(float v,float thickness){return 1.0-smoothstep(thickness,thickness+fwidth(v),abs(v));}
uniform bool observatory;
uniform bool calibrated;
uniform float observationProgress;
uniform int regionCount;
uniform vec4 regionBounds[8];
uniform int regionMaterial[8];
uniform vec3 observerEye;
uniform vec3 landmarkPosition;
uniform vec3 exitCenter;
uniform int guideCount;
uniform vec3 guidePoints[16];
uniform int obstacleCount;
uniform vec4 obstacleRects[16];
float segmentDistance(vec2 p,vec2 a,vec2 b){vec2 d=b-a;return length(p-a-d*clamp(dot(p-a,d)/max(dot(d,d),.001),0.,1.));}
vec3 observatorySurface(vec3 n,vec3 illumination){
    int zone=4;
    for(int i=0;i<regionCount;++i){vec4 r=regionBounds[i];if(all(greaterThanEqual(FragPos.xz,r.xy))&&all(lessThanEqual(FragPos.xz,r.zw)))zone=regionMaterial[i];}
    float grain=hash(floor(FragPos.xz*2.0)+floor(FragPos.y));
    vec3 stone=vec3(.13,.082,.069);
    if(materialKind==4)stone=vec3(.46,.55,.50);
    if(materialKind==5)stone=vec3(.035,.075,.14);
    if(materialKind==6)stone=vec3(.43,.45,.38);
    if(materialKind==1){
        stone=zone==0?vec3(.12,.075,.06):zone==1?vec3(.49,.60,.54):zone==2?vec3(.028,.065,.115):zone==3?vec3(.18,.24,.25):vec3(.10,.12,.17);
        float layer=sin(FragPos.x*.43+sin(FragPos.z*.21)*2.5);
        stone*=.91+.035*grain+.055*step(.25,layer);
        if(zone==1){
            // Broad dry salt terraces, no glittering subpixel reflection.
            float salt=abs(fract((FragPos.x+sin(FragPos.z*.23)*3.)*.13)-.5);
            stone*=.8+.2*smoothstep(.02,.08,salt);
        } else if(zone==2){
            float stripe=abs(fract((FragPos.z+FragPos.x*.25)*.20)-.5);
            stone+=line(stripe-.19,.018)*vec3(.025,.055,.065);
        }
    }else{
        vec2 uv=abs(n.x)>.5?FragPos.zy:FragPos.xy;
        float strata=abs(fract((uv.y+floor(uv.x*.23)*.32)*.22)-.5);
        stone*=.81+.035*grain+.10*step(.16,strata);
        float shoulder=1.-smoothstep(.04,.22,abs(FragPos.y-brushMinimum.y-brushSize.y));
        stone+=shoulder*vec3(.07,.09,.10);
        if(materialKind==5){
            vec3 local=(FragPos-brushMinimum)/brushSize;
            vec2 facetUV=vec2(abs(n.x)>.5?local.z:local.x,local.y);
            float cut=facetUV.x+facetUV.y*.34;
            stone*=.72+.42*step(.43,cut)-.20*step(.85,cut);
            float fracture=min(abs(cut-.43),abs(cut-.85));
            stone+=line(fracture,.006)*vec3(.06,.16,.20);
            float sheet=abs(fract(uv.y*.13+uv.x*.025)-.5);
            stone+=line(sheet,.01)*vec3(.006,.016,.025);
        }
    }
    float facing=.62+.38*max(dot(n,normalize(vec3(-.4,.8,.3))),0.);
    vec3 result=stone*(vec3(.62)*facing+illumination*.42);
    if(materialKind==1){
        float contact=1.;
        for(int i=0;i<obstacleCount;++i){vec4 r=obstacleRects[i];vec2 d=max(max(r.xy-FragPos.xz,FragPos.xz-r.zw),vec2(0));contact=min(contact,.56+.44*smoothstep(0.,1.4,length(d)));}
        result*=contact;
        float path=10.;
        for(int i=1;i<guideCount;++i)path=min(path,segmentDistance(FragPos.xz,guidePoints[i-1].xz,guidePoints[i].xz));
        float guide=line(path-.38,.025);
        result+=guide*mix(vec3(.025,.065,.060),vec3(.10,.25,.18),float(calibrated));
        float pad=length(FragPos.xz-observerEye.xz);
        float mark=line(pad-.82,.045)+line(pad-1.05,.025);
        result+=mark*mix(vec3(.15,.20,.18),vec3(.36,.48,.32),max(observationProgress,float(calibrated)));
        float exitRing=abs(max(abs(FragPos.x-exitCenter.x),abs(FragPos.z-exitCenter.z))-1.6);
        result+=line(exitRing,.07)*mix(vec3(.02,.04,.045),vec3(.22,.48,.31),float(calibrated));
    }
    return clamp(result,0.,.82);
}

uniform bool combatWorld;
uniform vec3 textureMean;
uniform float textureWorldScale;
uniform float textureStrength;
vec3 combatTexture(vec3 n){
    // Models intentionally have no authored UVs. Continuous world projections
    // keep metre-scale texel density on curved walls, slopes and inclined beams.
    vec3 weights=pow(abs(n),vec3(8.));weights/=max(dot(weights,vec3(1.)),.001);
    vec3 p=FragPos*textureWorldScale;
    vec3 sampleColor=texture(texture1,p.zy).rgb*weights.x+
                     texture(texture1,p.xz).rgb*weights.y+
                     texture(texture1,p.xy).rgb*weights.z;
    // Asset mean normalization preserves regional brightness and the warm
    // threat palette. This is bounded albedo modulation, not baked lighting.
    vec3 ratio=clamp(sampleColor/max(textureMean,vec3(.02)),vec3(.55),vec3(1.45));
    return mix(vec3(1.),ratio,textureStrength);
}
vec3 combatSurface(vec3 n,vec3 illumination){
    vec3 base=objectColor;
    float upward=max(n.y,0.0);
    vec2 uv=abs(n.y)>.65?FragPos.xz:abs(n.x)>.5?FragPos.zy:FragPos.xy;
    float strata=abs(fract((uv.y+floor(uv.x*.18)*.23)*.18)-.5);
    float facet=.86+.14*step(.2,strata);
    if(materialKind==10){
        base=vec3(.15,.20,.235);
        for(int i=0;i<regionCount;++i){
            vec4 r=regionBounds[i];int k=regionMaterial[i];
            if(k==2)continue;
            if(all(greaterThanEqual(FragPos.xz,r.xy))&&all(lessThanEqual(FragPos.xz,r.zw)))
                base=k==1?vec3(.06,.11,.15):k==3?vec3(.19,.115,.083):k==4?vec3(.36,.45,.40):vec3(.15,.20,.235);
        }
        float plate=min(abs(fract(uv.x*.125)-.5),abs(fract(uv.y*.125)-.5));
        facet*=.8+.2*smoothstep(.006,.022,plate);
    }
    if(materialKind==11){
        float radial=length(FragPos.xz-floorCenter);
        float ribs=abs(fract(radial*.32)-.5);
        facet*=.82+.18*smoothstep(.02,.06,ribs);
    }
    if(materialKind==12){
        // Coarse internal facets aligned with the actual inclined shell faces.
        float cut=uv.x*.18+uv.y*.085;
        facet=.65+.30*step(.3,fract(cut))+.15*step(.72,fract(cut));
    }
    if(useTexture){
        base*=combatTexture(n);
        facet=mix(facet,1.,.75); // Don't stack the old broad bands over the tile.
    }
    float facing=.55+.45*max(dot(n,normalize(vec3(-.35,.8,.4))),0.);
    vec3 result=base*facet*(vec3(.65)*facing+illumination*.40);
    if(materialKind==12){
        float seam=abs(fract(uv.x*.18+uv.y*.085)-.3);
        result+=line(seam,.008)*mix(vec3(.012,.055,.065),vec3(.05,.025,.09),phase);
    }
    if(materialKind==11 && upward>.7){
        // Broad pale inlays communicate a traversable orbital deck.
        float radial=length(FragPos.xz-floorCenter);
        float track=min(abs(radial-12.55),abs(radial-17.45));
        result+=line(track,.045)*vec3(.09,.14,.14);
    }
    result*=mix(vec3(.92,1.02,1.04),vec3(1.10,.82,1.17),phase);
    if(upward>.7){
        for(int i=0;i<4;++i){
            if(!riftActive[i])continue;
            vec3 local=(riftWorldToLocal[i]*vec4(FragPos,1.)).xyz;
            if(abs(local.y)>.15)continue;
            float radius=length(local.xz);
            float ring=line(radius-1.15,.045);
            result+=ring*mix(vec3(.03,.11,.14),vec3(.09,.04,.14),phase);
            float p=riftWarnings[i];
            if(p>=0.)result+=(line(radius-(1.7-.5*p),.08)+ring*.4)*vec3(.85,.36,.07)*(.65+.35*sin(p*25.13));
        }
    }
    return clamp(result,0.,.82);
}

void main(){
    vec3 n=normalize(Normal);
    vec3 base=objectColor;
    if(useTexture) base*=texture(texture1,TexCoord).rgb;
    float mineral=hash(floor(FragPos.xz*.9)+floor(FragPos.y*1.5));
    base*=.82+.22*mineral;
    vec3 illum=ambientLight;
    if(lightingEnabled) for(int i=0;i<numLights && i<128;++i){
        vec3 delta=lights[i].position-FragPos;
        float dist=length(delta);
        float att=pow(clamp(1.0-dist/max(lights[i].range,.001),0.0,1.0),2.0);
        float lambert=.3+.7*max(0.0,dot(n,delta/max(dist,.001)));
        illum+=lights[i].color*lights[i].intensity*att*lambert;
    }
    if(combatWorld){FragColor=vec4(combatSurface(n,illum),1.);return;}
    if(observatory){FragColor=vec4(observatorySurface(n,illum),1.);return;}
    vec3 result=base*illum;
    // Cool illumination changes settle permanently; no exposure flash or geometry change.
    result*=mix(vec3(.92,1.05,1.1),vec3(1.1,.82,1.18),phase);
    if(materialKind==1){
        vec2 floorPosition=FragPos.xz-floorCenter;
        float seam=min(abs(fract(floorPosition.x*.25)-.5),abs(fract(floorPosition.y*.25)-.5));
        float grid=1.0-smoothstep(.006,.02,seam);
        result+=grid*vec3(.012,.018,.035);
        vec2 outlineExtent=max(floorHalfExtent-vec2(5.0),floorHalfExtent*.25);
        float outer=abs(max(abs(floorPosition.x)-outlineExtent.x,abs(floorPosition.y)-outlineExtent.y));
        result+=line(outer,.028)*mix(vec3(.015,.058,.065),vec3(.045,.026,.07),phase);
        for(int i=0;i<4;++i){
            if(!riftActive[i]) continue;
            vec3 local=(riftWorldToLocal[i]*vec4(FragPos,1.0)).xyz;
            if(abs(local.y)>.15) continue;
            vec2 offset=local.xz;float radius=length(offset);
            float ring=line(radius-1.15,.045)+.4*line(radius-.85,.025);
            float spokes=line(min(abs(offset.x),abs(offset.y)),.035)*step(.85,radius)*step(radius,1.5);
            vec3 cold=mix(vec3(.025,.13,.17),vec3(.09,.045,.17),phase);
            result+=(ring+spokes)*cold;
            float p=riftWarnings[i];
            if(p>=0.0){
                float pulse=.65+.35*sin(p*25.13);
                float disk=(1.0-smoothstep(1.2,1.8,radius))*.12;
                float warning=line(radius-(1.7-.5*p),.08);
                result+=(warning+disk+ring*.4)*vec3(.85,.36,.07)*pulse;
            }
        }
    }
    if(materialKind==2){
        // Fixed motif coordinates belong to this preset, not a second world
        // obstacle: normalize the actual compiled brush bounds before shading.
        vec3 local=(FragPos-brushMinimum)/brushSize;
        float surface = ((abs(n.x)>.5 ? local.z : local.x)-.5)*4.0;
        float height=local.y*4.2;
        float split = fract(surface*.65+floor(height*.4)*.23);
        float facet = .5+.5*sin(height*2.6+surface*1.1);
        result *= .65+.6*step(split,.45+.18*sin(height*1.3));
        float fracture=line(split-.48,.01);
        result+=fracture*mix(vec3(.035,.16,.2),vec3(.09,.055,.2),phase);
        float vein=pow(facet,16.0);
        float breathe=.8+.2*sin(time*.9+height*.5);
        result+=mix(vec3(.015,.16,.19),vec3(.065,.065,.2),phase)*(.35+vein*.55)*breathe;
    }
    FragColor=vec4(clamp(result,0.0,.85),1.0);
}
