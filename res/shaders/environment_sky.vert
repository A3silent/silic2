#version 330 core
out vec3 worldRay;
uniform mat4 inverseProjection;
uniform mat3 inverseViewRotation;
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2) * 2.0 - 1.0;
    vec4 ray = inverseProjection * vec4(p,1.0,1.0);
    worldRay = inverseViewRotation * ray.xyz;
    gl_Position = vec4(p,1.0,1.0);
}
