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
