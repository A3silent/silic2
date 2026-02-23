#version 330 core
layout(location = 0) in vec2 pixelOffset;

uniform vec2 screenSize;

void main() {
    // Convert pixel offset from screen center to NDC
    vec2 ndc = pixelOffset / (screenSize * 0.5);
    gl_Position = vec4(ndc, 0.0, 1.0);
}
