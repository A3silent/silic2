#version 330 core
layout(location = 0) in vec2 aPos;  // pixel coords, origin = top-left of screen

uniform vec2 screenSize;

void main() {
    vec2 ndc;
    ndc.x =  (aPos.x / screenSize.x) * 2.0 - 1.0;
    ndc.y = -(aPos.y / screenSize.y) * 2.0 + 1.0;  // flip Y (OpenGL NDC has Y-up)
    gl_Position = vec4(ndc, 0.0, 1.0);
}
