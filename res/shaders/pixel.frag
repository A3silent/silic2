#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec2 resolution;

// Sixteen perceptually spaced levels retain the limited palette while giving
// dark mineral surfaces and the sky more than one usable shadow step.
const float colorLevels = 16.0;

// Dithering pattern (Bayer 4x4)
const mat4 ditherMatrix = mat4(
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0
) / 16.0;

vec3 quantizeColor(vec3 color, vec2 fragCoord) {
    ivec2 pos = ivec2(fragCoord) % 4;
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    // No shadow checkerboard. Mid/highlights receive only a small fraction of
    // a quantization step, centered on zero instead of adding a brightness bias.
    float dither = (ditherMatrix[pos.x][pos.y] - 0.46875)
                 * 0.18 * smoothstep(0.05, 0.30, luminance);
    vec3 perceptual = sqrt(clamp(color, 0.0, 1.0));
    vec3 quantized = clamp(floor(perceptual * (colorLevels - 1.0) + 0.5 + dither)
                          / (colorLevels - 1.0), 0.0, 1.0);
    return quantized * quantized;
}

vec3 applyCRTEffect(vec3 color, vec2 uv) {
    // Multiplicative 0..1.5% attenuation: black remains black and scanlines
    // cannot become brighter than the scene they decorate.
    float scanline = 0.5 + 0.5 * cos(uv.y * resolution.y * 6.2831853);
    color *= 1.0 - 0.015 * scanline;
    vec2 offset = uv - 0.5;
    color *= 1.0 - 0.12 * dot(offset, offset); // At most 6% corner attenuation.
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