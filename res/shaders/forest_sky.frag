#version 330 core
in vec3 worldRay;
out vec4 FragColor;
uniform float phase;
uniform float time;
const float PI=3.14159265359;
float hash(vec3 p){return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453);}
float noise(vec3 p){vec3 a=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(mix(hash(a),hash(a+vec3(1,0,0)),f.x),mix(hash(a+vec3(0,1,0)),hash(a+vec3(1,1,0)),f.x),f.y),mix(mix(hash(a+vec3(0,0,1)),hash(a+vec3(1,0,1)),f.x),mix(hash(a+vec3(0,1,1)),hash(a+vec3(1,1,1)),f.x),f.y),f.z);}
float fbm(vec3 p){return .55*noise(p)+.28*noise(p*2.03)+.12*noise(p*4.13);}
void main(){
 vec3 d=normalize(worldRay);
 // Keep the broad sky dim; expose the stars and iris independently.
 float horizon=pow(1.-abs(d.y),4.);
 vec3 color=mix(vec3(.008,.014,.036),vec3(.022,.008,.046),phase);
 color+=horizon*mix(vec3(.025,.034,.057),vec3(.042,.019,.06),phase);
 float ribbon=exp(-15.*abs(d.y-.22-.17*sin(d.x*4.+d.z*3.)));
 float cloud=.5+.5*sin(d.x*18.+sin(d.z*13.)*2.+d.y*23.);
 color+=ribbon*(.35+.65*cloud)*mix(vec3(.015,.014,.043),vec3(.033,.01,.049),phase);
 color*=.24;
 vec3 cell=floor(d*190.);
 if(hash(cell)>=.994){
  // Fixed star cells, with independent phase and speed for their twinkle.
  float rate=mix(.33,.80,hash(cell+73.));
  float twinkle=1.+.44*sin(2.*PI*(time*rate+hash(cell+113.)));
  color+=twinkle*(.17+.30*hash(cell+31.))*vec3(.73,.83,1.);
 }
 // Offset the northern eye from the spire so its pupil reads at player height.
 vec3 axis=normalize(vec3(.36,.25,-1.));
 vec3 right=normalize(cross(axis,vec3(0,1,0))),up=cross(axis,right);
 float front=dot(d,axis);vec2 q=vec2(dot(d,right),dot(d,up))/max(front,.1);
 // Slow and soften the eye motion independently of stars and clouds.
 float eyeTime=time*.20;
 const float eyeAmplitude=.40;
 vec2 tremble=eyeAmplitude*vec2(.0043*sin(eyeTime*4.3)+.0025*sin(eyeTime*8.7),
                               .0032*sin(eyeTime*5.7)+.0019*sin(eyeTime*11.3));
 vec2 contraction=vec2(.012*sin(eyeTime*4.4),.009*sin(eyeTime*3.5))
                  +vec2(.006,-.005)*sin(eyeTime*7.6);
 vec2 eyeScale=.85*(vec2(1.)+eyeAmplitude*contraction);
 vec2 eyeP=(q-tremble)/eyeScale;
 float r=length(eyeP),a=atan(eyeP.y,eyeP.x),radius=.255;
 float eye=1.-smoothstep(radius-.003,radius+.003,r);
 float normalized=r/radius;
 float pupil=1.-smoothstep(.27,.285,normalized);
 // Soft radial colour gradient, with irregular filaments rather than gear rings.
 float cloudSeed=fbm(vec3(eyeP*19.,2.1));
 float warpedAngle=a+.11*sin(normalized*9.+cloudSeed*5.);
 vec3 strandDomain=vec3(cos(warpedAngle)*41.,sin(warpedAngle)*41.,normalized*4.6);
 float strands=pow(1.-abs(noise(strandDomain)*2.-1.),2.4);
 float veil=fbm(vec3(eyeP*32.,4.));
 float gradient=clamp(normalized+(cloudSeed-.5)*.12,0.,1.);
 vec3 ivory=vec3(.57,.48,.48),rose=vec3(.27,.14,.32),indigo=vec3(.045,.085,.20);
 vec3 iris=mix(ivory,rose,smoothstep(.32,.71,gradient));
 iris=mix(iris,indigo,smoothstep(.68,.99,gradient));
 iris=mix(iris,vec3(.09,.20,.26),smoothstep(.60,.88,gradient)*smoothstep(.54,.78,veil)*.6);
 float cyanRibbon=exp(-pow((normalized-(.70+.09*sin(a*2.+1.)))/.13,2.));
 cyanRibbon*=smoothstep(.42,.69,fbm(vec3(eyeP*13.+5.,6.)));
 iris=mix(iris,vec3(.10,.24,.30),cyanRibbon*.42);
 iris*=.61+.39*strands;
 iris+=exp(-abs(normalized-.335)*21.)*vec3(.065,.055,.07);
 iris*=1.-smoothstep(.88,1.06,normalized);
 // Compress iris highlights without clipping their hue or lifting the pupil.
 float irisPeak=max(iris.r,max(iris.g,iris.b));
 iris*=.95/(1.+.80*irisPeak);
 iris=mix(iris,vec3(.002,.003,.01)*.24,pupil);
 if(front>0.)color=mix(color,iris,eye);
 // Turbulent domain-warped clouds; broad, uneven ribbons, no repeated circular rails.
 float drift=time*.0015;
 float cloudR=length(q),cloudA=atan(q.y,q.x);
 float swirlAngle=cloudA+.55*sin(cloudR*7.);
 vec2 swirl=vec2(cos(swirlAngle),sin(swirlAngle));
 vec2 warp=vec2(fbm(vec3(q*9.,2.+drift)),fbm(vec3(q*9.+13.,6.-drift)))-.45;
 vec2 cloudP=q+warp*.15+swirl*.035;
 float rolling=fbm(vec3(cloudP*12.,3.));
 float depth=fbm(vec3(cloudP*27.+warp*2.,7.));
 float filament=1.-abs(2.*fbm(vec3(cloudP*39.,4.))-1.);
 float envelope=exp(-pow((cloudR-.335)/.17,2.));
 // Fit the veil to the smaller eye's edge without following its tremor.
 float frontVeil=smoothstep(.222*.85,.277*.85,cloudR);
 float folds=smoothstep(.30,.64,rolling)*envelope*frontVeil;
 folds=max(folds,filament*.22*envelope*frontVeil);
 vec3 vapor=mix(vec3(.018,.026,.052),vec3(.17,.11,.23),smoothstep(.29,.65,depth));
 vapor=mix(vapor,vec3(.11,.21,.28),smoothstep(.59,.80,rolling)*.65);
 vapor+=pow(max(filament-.55,0.)*2.2,2.)*vec3(.08,.065,.11)*envelope;
 if(front>0.)color=mix(color,vapor*.34,clamp(folds*.91,0.,.90));
 color*=mix(vec3(1.),vec3(1.05,.91,1.12),phase);
 FragColor=vec4(clamp(color,0.,1.),1.);
}
