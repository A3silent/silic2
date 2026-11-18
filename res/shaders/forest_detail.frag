#version 330 core
in vec3 worldPosition;
in vec3 faceNormal;
in float tint;
uniform float phase;
out vec4 FragColor;
void main(){
    float distance=length(worldPosition.xz);
    float fog=smoothstep(125.,265.,distance);
    vec3 pine=mix(vec3(.006,.012,.015),vec3(.009,.016,.026),fog);
    pine*=.84+.16*fract(tint*1.7);
    pine*=mix(vec3(1.),vec3(1.06,.91,1.12),phase);
    FragColor=vec4(pine,1.);
}
