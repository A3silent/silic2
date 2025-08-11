#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 bulletColor;
uniform float intensity;

void main() {
    // Emissive effect - bullet self-illumination
    vec3 emissiveColor = bulletColor * intensity;
    
    // Add some rim lighting effect
    vec3 viewDir = normalize(-FragPos);
    float rim = 1.0 - max(0.0, dot(normalize(Normal), viewDir));
    rim = pow(rim, 2.0);
    
    // Final color = base emission + rim highlight
    vec3 finalColor = emissiveColor + bulletColor * rim * 0.5;
    
    // Output color, unaffected by lighting (self-emissive)
    FragColor = vec4(finalColor, 1.0);
}