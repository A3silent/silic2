#version 330 core
in vec2 mmNorm;
out vec4 FragColor;

uniform vec4  color;
uniform float innerRadius; // 0 = fill whole circle; >0 = only draw ring [inner..1]

void main() {
    float d = length(mmNorm);
    if (d > 1.0) discard;
    if (innerRadius > 0.0 && d < innerRadius) discard;
    FragColor = color;
}
