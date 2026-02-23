#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 enemyColor;

void main() {
    vec3 norm = normalize(Normal);

    // Half-Lambert: wraps lighting to avoid fully-black faces
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5));
    float diff = dot(norm, lightDir) * 0.5 + 0.5; // [0, 1] range

    // High minimum brightness so enemies are always visible regardless of ambient
    float lighting = mix(0.55, 1.0, diff);

    vec3 result = clamp(enemyColor * lighting, 0.0, 1.0);
    FragColor = vec4(result, 1.0);
}
