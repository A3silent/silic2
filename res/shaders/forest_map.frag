#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
out vec4 FragColor;
uniform vec3 objectColor;
uniform vec3 ambientLight;
uniform vec3 cameraPosition;
uniform bool lightingEnabled;
uniform bool useTexture;
uniform sampler2D texture1;
uniform bool useForestGroundTexture;
uniform int materialKind;
uniform float phase;
uniform float time;
uniform vec4 riftWarnings;
uniform bool riftActive[4];
uniform mat4 riftWorldToLocal[4];
struct Light{vec3 position;vec3 color;float intensity;float range;};
uniform int numLights;
uniform Light lights[128];
const float PI=3.14159265359;
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);}
float segment(vec2 p,vec2 a,vec2 b){vec2 q=p-a,v=b-a;return length(q-v*clamp(dot(q,v)/max(dot(v,v),.0001),0.,1.));}
float lineInk(float d,float width){return 1.-smoothstep(width,width+max(fwidth(d),.015),d);}
float glyph(vec2 p,float seed){
 // Fictional nonalphabetic sigils: asymmetric bent stems, hooks, branches, dots.
 float h=hash(vec2(seed,21.));float bend=(h-.5)*.5;
 float d=segment(p,vec2(-.12,-.73),vec2(bend,.1));
 d=min(d,segment(p,vec2(bend,.1),vec2(.13,.73)));
 d=min(d,segment(p,vec2(bend,-.05),vec2(-.5,.33)));
 d=min(d,segment(p,vec2(-.5,.33),vec2(-.27,.6)));
 d=min(d,segment(p,vec2(bend,.22),vec2(.47,-.12)));
 if(h>.3)d=min(d,segment(p,vec2(.47,-.12),vec2(.30,-.44)));
 if(h>.65)d=min(d,abs(length(p-vec2(.02,-.45))-.23));
 return lineInk(d,.045);
}
vec3 illumination(vec3 n){
 vec3 light=ambientLight*(.68+.32*max(n.y,0.));
 for(int i=0;i<min(numLights,128);++i){
  vec3 v=lights[i].position-FragPos;float d=length(v);
  float att=pow(max(1.-d/max(lights[i].range,.01),0.),2.);
  float diffuse=.12+.88*max(dot(n,v/max(d,.001)),0.);
  light+=lights[i].color*lights[i].intensity*att*diffuse;
 }
 return light;
}
void main(){
 vec3 n=normalize(Normal);vec3 base=objectColor;
 bool sampleSlate=materialKind==20 && useTexture && useForestGroundTexture;
 if(useTexture && !sampleSlate)base*=texture(texture1,TexCoord).rgb;
 vec3 emission=vec3(0.);
 if(materialKind==20){
  if(sampleSlate){
   // Repeat every 10 world units, including across adjacent floor brushes.
   // Normalize the dark image to the authored floor tint before lighting.
   base*=texture(texture1,FragPos.xz*.10).rgb*4.;
  }else{
   // Original limestone when the built-in slate asset isn't available.
   float broad=noise(FragPos.xz*.17),grain=noise(FragPos.xz*3.7);
   float fine=hash(floor(FragPos.xz*12.));
   float veins=pow(1.-abs(sin(FragPos.x*.29+FragPos.z*.18+broad*7.)),14.);
   base*=.78+.25*broad+.12*grain;
   base=mix(base,base*.58,veins*.35);
   base*=1.-step(.965,fine)*.19;
  }
  // Rifts retain their existing warm combat warning; no extra scenery lamp.
  for(int i=0;i<4;++i)if(riftActive[i] && riftWarnings[i]>=0.){
   vec3 p=(riftWorldToLocal[i]*vec4(FragPos,1)).xyz;
   float circle=lineInk(abs(length(p.xz)-1.9),.13);
   emission+=circle*vec3(.68,.20,.025)*(.65+.35*sin(riftWarnings[i]*25.13));
  }
 }else if(materialKind==23){
  // Broad facets and irregular luminous fictional script on the actual spire.
  float angle=atan(FragPos.z,FragPos.x)+FragPos.y*.011;
  float lane=(angle+PI)/(2.*PI)*7.;float row=floor(FragPos.y/6.2);
  vec2 local=vec2((fract(lane)-.5)*2.,(fract(FragPos.y/6.2)-.5)*2.);
  float rune=glyph(local,floor(lane)+row*13.);
  float winding=abs(sin(angle*5.+FragPos.y*.18+sin(FragPos.y*.071)*1.8));
  float fissure=1.-smoothstep(.025,.065,winding);
  base*=.6+.4*noise(vec2(angle*7.,FragPos.y*.17));
  float facet=.4+.6*abs(n.x*.67+n.z*.73);
  float edge=pow(1.-abs(dot(n,normalize(cameraPosition-FragPos))),2.);
  float mineral=.7+.3*noise(vec2(angle*9.,FragPos.y*.055));
  // The central crystal itself emits the restrained cold body illumination.
  emission=(vec3(.065,.078,.16)*facet+vec3(.04,.055,.12)*edge)*mineral;
  emission+=(rune*.30+fissure*.12)*mix(vec3(.26,.25,.55),vec3(.36,.21,.52),phase);
  emission+=vec3(.025,.02,.07)*(1.-smoothstep(0.,20.,FragPos.y));
 }else if(materialKind==22){
  float leaves=noise(FragPos.xz*1.9+FragPos.y*.7);
  float woody=noise(vec2(atan(FragPos.z,FragPos.x)*240.,FragPos.y*.4));
  base=mix(vec3(.23,.19,.15),vec3(.13,.20,.14),smoothstep(1.3,4.2,FragPos.y));
  base*=.65+.25*leaves+.22*woody;
 }else{
  base*=.8+.2*noise(FragPos.xz*.8+FragPos.y*.2);
 }
 vec3 light=illumination(n);
 // Leaf and bark receive diffuse sky fill, without adding a second light source.
 if(materialKind==21)light+=ambientLight*1.25;
 vec3 color=base*light+emission;
 color*=mix(vec3(1.),vec3(1.05,.90,1.10),phase);
 // Cool sky haze belongs to ambience, not a hidden directional light.
 float distance=length(FragPos.xz);float fog=smoothstep(108.,220.,distance)*.68;
 color=mix(color,vec3(.004,.008,.017),fog);
 FragColor=vec4(clamp(color,0.,.92),1.);
}
