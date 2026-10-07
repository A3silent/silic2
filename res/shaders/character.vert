#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in float aEmission;
uniform mat4 view;
uniform mat4 projection;
out vec3 normal;
out vec3 color;
out float emission;
void main() {
    normal = aNormal;
    color = aColor;
    emission = aEmission;
    gl_Position = projection * view * vec4(aPosition, 1.0);
}
