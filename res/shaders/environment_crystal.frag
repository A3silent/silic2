#version 330 core
in vec3 worldPosition;
in vec3 faceNormal;
in float tint;
uniform float phase;
uniform float time;
uniform vec4 riftWarnings;
out vec4 FragColor;
void main(){
    if(tint<-.5){
        int id=int(-tint-.5);
        float p=riftWarnings[id];
        float tendon=.65+.35*sin(worldPosition.y*5.0+sin(time*.31)*.25);
        vec3 color=mix(vec3(.10,.105,.078),vec3(.09,.12,.068),phase)*tendon;
        if(p>=0.0)color=vec3(.9,.30,.055)*(.72+.28*sin(p*25.13));
        FragColor=vec4(color,1.0);return;
    }
    float light=.3+.7*max(0.0,dot(normalize(faceNormal),normalize(vec3(-.6,.8,.4))));
    float scale=.5+.5*sin(worldPosition.y*2.4+worldPosition.x*.7);
    vec3 iron=mix(vec3(.043,.048,.043),vec3(.075,.066,.050),scale*.4);
    vec3 bone=vec3(.19,.195,.145);
    vec3 base=tint>=1.0?mix(iron,bone,tint>=2.0?phase*.85:.50):iron;
    float seam=pow(.5+.5*sin(worldPosition.y*2.0+fract(tint)*6.0),24.0);
    vec3 result=base*light+seam*vec3(.017,.020,.013);
    FragColor=vec4(result,1.0);
}
