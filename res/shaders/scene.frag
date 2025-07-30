#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 objectColor;

// Quantize color to create pixel art effect
vec3 quantizeColor(vec3 color, float levels) {
    return floor(color * levels) / levels;
}

void main() {
    // Simple directional light
    vec3 lightDir = normalize(vec3(-0.2, -1.0, -0.3));
    vec3 lightColor = vec3(1.0, 0.95, 0.8);
    vec3 ambient = 0.3 * lightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Combine
    vec3 result = (ambient + diffuse) * objectColor;
    
    // Quantize for pixel style (8 levels)
    result = quantizeColor(result, 8.0);
    
    FragColor = vec4(result, 1.0);
}