#version 330 core
in vec3 normal;
in vec3 color;
in float emission;
out vec4 FragColor;
void main() {
    float facet = max(dot(normalize(normal), normalize(vec3(-0.4, 0.85, 0.3))), 0.0);
    float light = mix(0.52 + 0.48 * facet, 1.0, clamp(emission, 0.0, 1.0));
    FragColor = vec4(color * light, 1.0);
}
