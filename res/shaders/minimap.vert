#version 330 core
layout(location = 0) in vec2 worldPos; // world X, Z

uniform vec2  playerPos;   // player world X, Z
uniform float playerYaw;   // atan2(front.x, front.z) — forward maps to minimap +Y
uniform float viewRadius;  // world units shown within the minimap radius
uniform vec4  minimapRect; // (x, y from screen-bottom-left, w, h) in pixels
uniform vec2  screenSize;

out vec2 mmNorm; // normalised minimap coords [-1,1]: fed to frag for circle clip

void main() {
    vec2 rel = worldPos - playerPos;

    // Rotate so the player's forward direction always points to minimap top
    float c = cos(playerYaw);
    float s = sin(playerYaw);
    mmNorm = vec2(rel.x * c - rel.y * s,
                  rel.x * s + rel.y * c) / viewRadius;

    // [-1,1] → [0,1] → screen pixels → NDC
    vec2 mmPos     = mmNorm * 0.5 + 0.5;
    vec2 screenPos = minimapRect.xy + mmPos * minimapRect.zw;
    vec2 ndc       = screenPos / screenSize * 2.0 - 1.0;

    gl_Position = vec4(ndc, 0.0, 1.0);
}
