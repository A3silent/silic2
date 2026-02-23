#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 objectColor;
uniform vec3 ambientLight;
uniform bool lightingEnabled;
uniform bool useTexture;
uniform sampler2D texture1;

// Point lights
struct Light {
    vec3 position;
    vec3 color;
    float intensity;
    float range;
};

uniform Light lights[128];
uniform int numLights;


void main() {
    vec3 baseColor = objectColor;
    
    // Use texture color
    if (useTexture) {
        vec4 texColor = texture(texture1, TexCoord);
        baseColor = texColor.rgb * objectColor; // Blend texture color and object color
    }
    
    vec3 result = ambientLight * baseColor;
    
    if (lightingEnabled && numLights > 0) {
        vec3 norm = normalize(Normal);

        for (int i = 0; i < numLights && i < 128; i++) {
            vec3 lightVec = lights[i].position - FragPos;
            float distance = length(lightVec);

            // Quadratic attenuation: bright center, smooth fade to edge
            float attenuation = 0.0;
            if (lights[i].range > 0.0) {
                float t = clamp(distance / lights[i].range, 0.0, 1.0);
                attenuation = (1.0 - t) * (1.0 - t);
            }

            if (attenuation > 0.001) {
                vec3 lightDir = normalize(lightVec);

                // Half-Lambert so back-faces still catch some light
                float diff = dot(norm, lightDir) * 0.5 + 0.5;
                vec3 diffuse = diff * lights[i].color * lights[i].intensity * attenuation;

                result += diffuse * baseColor;
            }
        }
    }

    // Smooth brightness response: shadow floor ~0.35, continuous curve, no banding.
    // Highlight boost adds up to +0.3 for bright pixels only (activates above brightness 0.4).
    // Quantization is handled by pixel.frag (Bayer dithered, 16 levels).
    float brightness = dot(result, vec3(0.299, 0.587, 0.114));
    float mult = 0.35 + 0.65 * pow(brightness, 0.55) + 0.3 * smoothstep(0.4, 1.0, brightness);
    result *= mult;
    
    // Final clamp
    result = clamp(result, 0.0, 1.0);
    
    FragColor = vec4(result, 1.0);
}