#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 bulletColor;
uniform float intensity;

void main() {
    // 发光效果 - 子弹自身发光
    vec3 emissiveColor = bulletColor * intensity;
    
    // 添加一些边缘高亮效果
    vec3 viewDir = normalize(-FragPos);
    float rim = 1.0 - max(0.0, dot(normalize(Normal), viewDir));
    rim = pow(rim, 2.0);
    
    // 最终颜色 = 基础发光 + 边缘高亮
    vec3 finalColor = emissiveColor + bulletColor * rim * 0.5;
    
    // 输出颜色，不受光照影响（自发光）
    FragColor = vec4(finalColor, 1.0);
}