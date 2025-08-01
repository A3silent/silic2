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

// Quantize color to create pixel art effect (Eclipsium-style)
vec3 quantizeColor(vec3 color, float levels) {
    // More aggressive quantization for retro look
    return floor(color * levels + 0.5) / levels;
}

// Convert to limited palette (simulate 16-color EGA/VGA palette)
vec3 toPalette(vec3 color) {
    // Reduce to 2 bits per channel (4 levels)
    vec3 reduced = quantizeColor(color, 4.0);
    
    // Boost contrast
    reduced = pow(reduced, vec3(0.8));
    
    return reduced;
}

void main() {
    vec3 baseColor = objectColor;
    
    // 使用纹理颜色
    if (useTexture) {
        vec4 texColor = texture(texture1, TexCoord);
        baseColor = texColor.rgb * objectColor; // 混合纹理颜色和物体颜色
    }
    
    vec3 result = ambientLight * baseColor;
    
    if (lightingEnabled && numLights > 0) {
        vec3 norm = normalize(Normal);
        
        for (int i = 0; i < numLights && i < 128; i++) {
            vec3 lightDir = lights[i].position - FragPos;
            float distance = length(lightDir);
            
            // Attenuation
            float attenuation = 1.0;
            if (lights[i].range > 0.0) {
                attenuation = max(0.0, 1.0 - (distance / lights[i].range));
            }
            
            if (attenuation > 0.0) {
                lightDir = normalize(lightDir);
                
                // Diffuse lighting
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse = diff * lights[i].color * lights[i].intensity * attenuation;
                
                result += diffuse * baseColor;
            }
        }
    }
    
    // Apply retro palette conversion
    result = toPalette(result);
    
    // Hard lighting steps (no smooth gradients)
    float brightness = dot(result, vec3(0.299, 0.587, 0.114));
    if (brightness > 0.8) {
        result *= 1.2;  // Bright
    } else if (brightness > 0.5) {
        result *= 1.0;  // Normal
    } else if (brightness > 0.25) {
        result *= 0.7;  // Dark
    } else {
        result *= 0.4;  // Very dark
    }
    
    // Final clamp
    result = clamp(result, 0.0, 1.0);
    
    FragColor = vec4(result, 1.0);
}