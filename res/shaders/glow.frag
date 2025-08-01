#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 glowColor;
uniform float intensity;

void main() {
    // 计算到中心的距离
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(TexCoord, center);
    
    // 创建柔和的光晕效果
    float glow = 1.0 - smoothstep(0.0, 0.5, dist);
    glow = pow(glow, 2.0); // 使边缘更柔和
    
    // 添加内部更亮的核心
    float core = 1.0 - smoothstep(0.0, 0.2, dist);
    core = pow(core, 3.0);
    
    // 组合光晕和核心
    float finalGlow = glow * 0.6 + core * 0.4;
    
    // 应用颜色和强度
    vec3 color = glowColor * intensity * finalGlow;
    
    // 输出带透明度的颜色
    FragColor = vec4(color, finalGlow * 0.8);
}