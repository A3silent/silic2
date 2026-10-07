#version 330 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;
uniform vec2 logicalSize;
out vec4 vertexColor;
void main() {
    gl_Position = vec4(position.x / logicalSize.x * 2.0 - 1.0,
                       1.0 - position.y / logicalSize.y * 2.0, 0.0, 1.0);
    vertexColor = color;
}
