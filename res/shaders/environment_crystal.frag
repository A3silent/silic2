#version 330 core
in vec3 worldPosition;
in vec3 faceNormal;
in float tint;
uniform float phase;
uniform float time;
uniform vec4 riftWarnings;
out vec4 FragColor;
void main(){
    if(tint < -.5){
        int id=int(-tint-.5);
        float p=riftWarnings[id];
        vec3 color=mix(vec3(.025,.16,.2),vec3(.10,.045,.19),phase);
        if(p>=0.0) color=vec3(.9,.37,.075)*(.72+.28*sin(p*25.13));
        FragColor=vec4(color,1.0);return;
    }
    float facet=.4+.6*max(0.0,dot(normalize(faceNormal),normalize(vec3(-.6,.8,.4))));
    float vein=pow(.5+.5*sin(worldPosition.y*2.2+tint*8.0-time*.32),12.0);
    vec3 base=mix(vec3(.025,.12,.16),vec3(.065,.075,.17),phase);
    vec3 color=base*facet*(.75+tint*.4)+vein*mix(vec3(.01,.045,.05),vec3(.04,.015,.055),phase);
    FragColor=vec4(color,1.0);
}
