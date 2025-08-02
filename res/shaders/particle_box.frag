#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec3 ParticleColor;

out vec4 FragColor;

uniform vec3 viewPos;

void main() {
    // Simple rim lighting for glowing effect
    vec3 viewDir = normalize(viewPos - FragPos);
    float rim = 1.0 - max(dot(viewDir, normalize(Normal)), 0.0);
    rim = pow(rim, 2.0);
    
    // Base emissive color
    vec3 emissive = ParticleColor * 2.0;
    
    // Add rim glow
    vec3 finalColor = emissive + (ParticleColor * rim * 3.0);
    
    // Output with full opacity
    FragColor = vec4(finalColor, 1.0);
}