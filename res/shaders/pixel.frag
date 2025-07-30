#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec2 resolution;

// Color quantization levels (Eclipsium-style limited palette)
const float colorLevels = 16.0; // 16 levels per channel (4-bit color)

// Dithering pattern (Bayer 4x4)
const mat4 ditherMatrix = mat4(
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0
) / 16.0;

vec3 quantizeColor(vec3 color, vec2 fragCoord) {
    // Add dithering
    ivec2 pos = ivec2(fragCoord) % 4;
    float dither = ditherMatrix[pos.x][pos.y];
    
    // Quantize with dithering
    vec3 quantized = floor(color * colorLevels + dither) / colorLevels;
    
    return quantized;
}

// Simulate CRT/old monitor effect
vec3 applyCRTEffect(vec3 color, vec2 uv) {
    // Subtle scanline effect
    float scanline = sin(uv.y * resolution.y * 3.14159) * 0.04;
    color -= scanline;
    
    // Very subtle vignette
    float vignette = 1.0 - length(uv - 0.5) * 0.3;
    color *= vignette;
    
    return color;
}

void main() {
    vec3 color = texture(screenTexture, TexCoords).rgb;
    
    // Apply color quantization with dithering
    vec2 fragCoord = TexCoords * resolution;
    color = quantizeColor(color, fragCoord);
    
    // Apply CRT effect (optional, subtle)
    color = applyCRTEffect(color, TexCoords);
    
    // Ensure we stay in valid color range
    color = clamp(color, 0.0, 1.0);
    
    FragColor = vec4(color, 1.0);
}