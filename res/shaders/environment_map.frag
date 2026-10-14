#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
out vec4 FragColor;
uniform vec3 objectColor;
uniform vec3 ambientLight;
uniform bool lightingEnabled;
uniform int materialKind;
uniform float phase;
uniform float time;
uniform vec4 riftWarnings;
uniform bool riftActive[4];
uniform mat4 riftWorldToLocal[4];
uniform vec2 floorCenter;
uniform vec2 floorHalfExtent;
uniform vec3 brushMinimum;
uniform vec3 brushSize;
uniform bool useTexture;
uniform sampler2D texture1;
struct Light{vec3 position;vec3 color;float intensity;float range;};
uniform Light lights[128];
uniform int numLights;
uniform int obstacleCount;
uniform vec4 obstacleRects[16];
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise(vec2 p){
    vec2 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);
}
float line(float v,float thickness){return 1.0-smoothstep(thickness,thickness+max(fwidth(v),.001),abs(v));}
void main(){
    vec3 n=normalize(Normal);
    vec3 local=(FragPos-brushMinimum)/brushSize;
    vec2 surface=abs(n.y)>.7?FragPos.xz:(abs(n.x)>.5?FragPos.zy:FragPos.xy);
    float stain=noise(surface*.38);
    float grit=hash(floor(surface*6.0));
    float authored=dot(objectColor,vec3(.2126,.7152,.0722));
    vec3 base=mix(vec3(.11,.13,.13),vec3(.19,.165,.13),smoothstep(.4,.85,stain));
    base*=.72+authored*.9;
    base*=.85+.2*grit;
    if(useTexture)base*=texture(texture1,TexCoord).rgb;
    float ambient=dot(ambientLight,vec3(.2126,.7152,.0722));
    vec3 illum=ambient*mix(vec3(.78,.87,.83),vec3(.71,.84,.68),phase);
    if(lightingEnabled)for(int i=0;i<numLights && i<128;++i){
        vec3 delta=lights[i].position-FragPos;float dist=length(delta);
        float att=pow(clamp(1.0-dist/max(lights[i].range,.001),0.0,1.0),2.0);
        float lambert=.25+.75*max(0.0,dot(n,delta/max(dist,.001)));
        illum+=lights[i].color*lights[i].intensity*att*lambert;
    }
    vec3 result=base*illum;
    if(materialKind==1){
        vec2 p=FragPos.xz-floorCenter;
        vec2 plate=abs(fract(p*.25)-.5);
        float joint=1.0-smoothstep(.008,.023,min(plate.x,plate.y));
        result*=1.0-joint*.55;
        float contact=0.0;
        for(int i=0;i<obstacleCount && i<16;++i){
            vec4 bounds=obstacleRects[i];
            vec2 outside=max(bounds.xy-FragPos.xz,FragPos.xz-bounds.zw);
            float distance=max(outside.x,outside.y);
            contact=max(contact,1.0-smoothstep(.015,.22,abs(distance)));
        }
        result*=1.0-contact*.45;
        // Sparse worn inlay keeps the running loop readable without neon rails.
        vec2 extent=max(floorHalfExtent-vec2(5),floorHalfExtent*.25);
        float track=abs(max(abs(p.x)-extent.x,abs(p.y)-extent.y));
        float worn=step(.38,noise(p*1.6));
        result+=line(track,.045)*vec3(.055,.058,.045)*worn;
        float scar=line(sin(p.x*.71+p.y*.36+stain*.9),.018)*smoothstep(.5,.8,stain);
        result+=scar*vec3(.014,.021,.012);
        for(int i=0;i<4;++i){
            if(!riftActive[i])continue;
            vec3 r=(riftWorldToLocal[i]*vec4(FragPos,1)).xyz;
            if(abs(r.y)>.15)continue;
            float angle=atan(r.z,r.x+.0001),radius=length(r.xz);
            float edge=1.12+.1*sin(angle*3.0)+.05*cos(angle*7.0);
            float ring=line(radius-edge,.035)*(.5+.5*smoothstep(-.3,.7,sin(angle*5.0)));
            result+=ring*mix(vec3(.065,.070,.046),vec3(.060,.085,.040),phase);
            float progress=riftWarnings[i];
            if(progress>=0){
                float pulse=.65+.35*sin(progress*25.13);
                float disk=(1.0-smoothstep(1.2,1.8,radius))*.12;
                float warning=line(radius-(1.7-.5*progress),.08);
                result+=(warning+disk+ring*.5)*vec3(.85,.29,.045)*pulse;
            }
        }
    }else if(materialKind==2){
        // The fossil retains the exact authored silhouette. Ossified ribs,
        // recesses and crawling tissue are only a surface response.
        float s=(abs(n.x)>.5?local.z:local.x)*4.0;
        float h=local.y*4.2;
        float tissueNoise=noise(vec2(s*1.15,h*.9));
        float fold=sin(s*4.4+sin(h*1.35)*.8+.035*sin(time*.37+h*2.0));
        float cleft=1.0-smoothstep(.07,.20,abs(fold));
        float irregular=h*3.7+sin(s*1.6)*.65+tissueNoise*2.1;
        float boneSeam=line(sin(irregular),.055)*smoothstep(.36,.67,noise(vec2(s*1.8,h*1.25)));
        float calcified=smoothstep(.36,.68,tissueNoise);
        vec3 tissue=mix(vec3(.09,.105,.096),vec3(.52,.53,.435),calcified);
        result=tissue*illum*(1.0-cleft*.8);
        result+=boneSeam*vec3(.10,.103,.076)*(.3+.7*calcified);
        float tangentCoordinate=abs(n.x)>.5?local.z:local.x;
        float edge=1.0-smoothstep(.008,.055,min(tangentCoordinate,1.0-tangentCoordinate));
        float moon=max(0.0,dot(n,normalize(vec3(-.5,.8,.3))));
        result+=vec3(.10,.105,.085)*(edge*.55+max(n.y,0.0)*calcified*.35)*(.4+.6*moon);
        result*=mix(.55,1.0,smoothstep(0.0,.09,local.y));
        float pulse=.65+.35*sin(time*.31+h*1.7+sin(s*2.0));
        result+=cleft*vec3(.012,.024,.008)*pulse*(.3+.7*phase);
    }else{
        // Oxidized vertical plates and thin pale deposits distinguish cover
        // shoulders from the floor even when both share the dark palette.
        float run=sin(surface.x*3.1+stain*.8);
        float seam=line(run,.025)*smoothstep(.4,.8,stain);
        result+=seam*vec3(.038,.039,.029);
        float wornShoulder=smoothstep(.86,.99,local.y)*smoothstep(.2,.7,stain);
        result+=(max(n.y,0.0)*.55+wornShoulder)*vec3(.10,.105,.087);
        result*=mix(.65,1.0,smoothstep(0.0,.07,local.y));
    }
    FragColor=vec4(clamp(result,0.0,.85),1.0);
}
