#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;
uniform mat4 model;
uniform mat4 projection;
out vec3 n;
out vec2 texcoord;
out vec3 localPosition;
out vec3 cameraPosition;
void main(){n=transpose(inverse(mat3(model)))*normal;texcoord=uv;localPosition=position;cameraPosition=(model*vec4(position,1)).xyz;gl_Position=projection*model*vec4(position,1);}
