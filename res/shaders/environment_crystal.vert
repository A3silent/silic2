#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in float variation;
uniform mat4 view;
uniform mat4 projection;
out vec3 worldPosition;
out vec3 faceNormal;
out float tint;
void main(){worldPosition=position;faceNormal=normal;tint=variation;gl_Position=projection*view*vec4(position,1.0);}
