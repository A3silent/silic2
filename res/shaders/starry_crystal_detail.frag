#version 330 core
in vec3 worldPosition;
in vec3 faceNormal;
in float tint;
uniform float phase;
uniform float time;
uniform vec4 riftWarnings;
out vec4 FragColor;
uniform bool observatory;
uniform bool combatWorld;
uniform bool calibrated;
uniform float observationProgress;
uniform vec3 landmarkPosition;
uniform vec3 landmarkScale;
uniform vec3 cameraPosition;
float grain(vec3 p){return fract(sin(dot(p,vec3(127.1,311.7,73.1)))*43758.5453);}
void main(){
    if(combatWorld && tint>=50.){
        FragColor=vec4(mix(vec3(.09,.15,.17),vec3(.13,.08,.20),phase)*(tint<51.?1.:.6),1.);return;
    }
    if(observatory){
        float facing=.45+.55*abs(dot(normalize(faceNormal),normalize(vec3(-.4,.8,.2))));
        vec3 color=vec3(.08,.12,.15)*facing;
        if(tint>=10. && tint<13.){
            color=mix(vec3(.30,.44,.40),vec3(.56,.69,.45),max(observationProgress,float(calibrated)));
        }else if(tint>=20. && tint<21.){
            vec3 p=(worldPosition-landmarkPosition)/landmarkScale;
            float cleft=step(.25,sin(p.y*18.+p.x*4.));
            color=mix(vec3(.10,.20,.24),vec3(.23,.29,.31),cleft)*facing;
            if(calibrated){
                vec3 direction=normalize(worldPosition-cameraPosition);
                float stars=step(.988,grain(floor(direction*140.)));
                float band=exp(-12.*abs(direction.y+.17*sin(direction.x*4.)));
                color=vec3(.017,.012,.038)+vec3(.09,.035,.14)*band+stars*vec3(.4,.49,.56);
            }
        }else if(tint>=30. && tint<40.){
            color=vec3(.40,.44,.37)*facing;
            if(calibrated)color+=vec3(.08,.035,.13);
        }else if(tint>=40.)color=(tint<41.?vec3(.075,.05,.04):vec3(.09,.14,.17))*facing;
        FragColor=vec4(color,1.);return;
    }

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
