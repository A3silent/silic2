#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 enemyColor;
uniform vec3 ambientLight;

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
    float range;
};

uniform Light lights[128];
uniform int numLights;

void main() {
    vec3 norm = normalize(Normal);

    // Start with ambient
    vec3 result = ambientLight * enemyColor;

    for (int i = 0; i < numLights && i < 128; i++) {
        vec3 lightVec  = lights[i].position - FragPos;
        float distance = length(lightVec);

        // Quadratic attenuation matching map.frag
        float attenuation = 0.0;
        if (lights[i].range > 0.0) {
            float t = clamp(distance / lights[i].range, 0.0, 1.0);
            attenuation = (1.0 - t) * (1.0 - t);
        }

        if (attenuation > 0.001) {
            vec3 lightDir = normalize(lightVec);

            // Half-Lambert so back-faces catch some light
            float diff    = dot(norm, lightDir) * 0.5 + 0.5;
            vec3  diffuse = diff * lights[i].color * lights[i].intensity * attenuation;

            result += diffuse * enemyColor;
        }
    }

    // Same brightness curve as map.frag — smooth response, no quantization banding here
    float brightness = dot(result, vec3(0.299, 0.587, 0.114));
    float mult = 0.35 + 0.65 * pow(brightness, 0.55) + 0.3 * smoothstep(0.4, 1.0, brightness);
    result *= mult;

    FragColor = vec4(clamp(result, 0.0, 1.0), 1.0);
}
