#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 glowColor;
uniform float intensity;

void main() {
    // Calculate distance to center
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(TexCoord, center);
    
    // Create soft glow effect
    float glow = 1.0 - smoothstep(0.0, 0.5, dist);
    glow = pow(glow, 2.0); // Make edges softer
    
    // Add brighter inner core
    float core = 1.0 - smoothstep(0.0, 0.2, dist);
    core = pow(core, 3.0);
    
    // Combine glow and core
    float finalGlow = glow * 0.6 + core * 0.4;
    
    // Apply color and intensity
    vec3 color = glowColor * intensity * finalGlow;
    
    // Output color with transparency
    FragColor = vec4(color, finalGlow * 0.8);
}