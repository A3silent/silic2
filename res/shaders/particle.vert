#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in float aSize;

uniform mat4 view;
uniform mat4 projection;

out vec3 vertexColor;
out float vertexSize;

void main() {
    gl_Position = projection * view * vec4(aPos, 1.0);
    gl_PointSize = aSize * 20.0; // Scale up point size even more for visibility
    
    vertexColor = aColor;
    vertexSize = aSize;
}