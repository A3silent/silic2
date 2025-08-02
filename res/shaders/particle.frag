#version 330 core

in vec3 vertexColor;
in float vertexSize;

out vec4 FragColor;

void main() {
    // Calculate distance from center of point sprite
    vec2 coord = gl_PointCoord - vec2(0.5);
    float distance = length(coord);
    
    // Create circular particle with very soft edges for better glow
    float alpha = 1.0 - smoothstep(0.0, 0.5, distance);
    
    // Enhanced multi-layer glow effect
    float innerGlow = 1.0 - smoothstep(0.0, 0.3, distance);
    float midGlow = 1.0 - smoothstep(0.1, 0.5, distance);
    float outerGlow = 1.0 - smoothstep(0.3, 0.8, distance);
    
    // Create intense glowing core
    vec3 coreColor = vertexColor * 3.0; // Much brighter core
    vec3 glowColor = vertexColor * 2.0;  // Bright glow
    vec3 outerColor = vertexColor * 1.5; // Outer glow
    
    // Mix the layers for a complex glow effect
    vec3 finalColor = mix(outerColor, glowColor, midGlow);
    finalColor = mix(finalColor, coreColor, innerGlow * 0.7);
    
    // Add extra intensity to the whole particle
    finalColor *= 1.5;
    
    // Enhance alpha for better visibility
    alpha = alpha * 1.2;
    
    FragColor = vec4(finalColor, alpha);
    
    // Discard pixels that are too transparent
    if (FragColor.a < 0.01) {
        discard;
    }
}