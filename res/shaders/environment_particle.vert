#version 330 core
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform float phase;
uniform vec2 floorMinimum;
uniform vec2 floorMaximum;
uniform float floorY;
out float brightness;
float hash(float x){return fract(sin(x*127.1)*43758.5453);}
void main(){
    float id=float(gl_VertexID)+1.0;
    vec2 inset = min(vec2(.5), (floorMaximum-floorMinimum)*.1);
    vec2 ground = mix(floorMinimum+inset,floorMaximum-inset,vec2(hash(id),hash(id+37.0)));
    vec3 p=vec3(ground.x, floorY+.3+mod(hash(id+91.0)*7.0-time*.045+1000.0,7.0),ground.y);
    vec2 sway=min(vec2(.22),(floorMaximum-floorMinimum)*.05);
    p.x+=sin(time*.18+id)*sway.x;p.z+=cos(time*.15+id)*sway.y;
    gl_Position=projection*view*vec4(p,1.0);gl_PointSize=1.0;
    brightness=.35+.65*hash(id+3.0);
}
