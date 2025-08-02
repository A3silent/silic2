#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

// Per-instance data
layout (location = 2) in vec3 instancePos;
layout (location = 3) in vec3 instanceColor;
layout (location = 4) in float instanceSize;
layout (location = 5) in vec3 instanceVelocity;

uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec3 ParticleColor;

void main() {
    // Scale the box based on particle size (smaller than bullets)
    vec3 scaledPos = aPos * instanceSize * 0.5; // Half the size for better proportion
    
    // Create rotation matrix based on velocity direction
    vec3 forward = normalize(instanceVelocity);
    vec3 up = vec3(0.0, 1.0, 0.0);
    if (abs(dot(forward, up)) > 0.99) {
        up = vec3(1.0, 0.0, 0.0);
    }
    vec3 right = normalize(cross(up, forward));
    up = cross(forward, right);
    
    mat3 rotation = mat3(right, up, -forward);
    
    // Apply rotation and translation
    vec3 worldPos = instancePos + rotation * scaledPos;
    
    FragPos = worldPos;
    Normal = rotation * aNormal;
    ParticleColor = instanceColor;
    
    gl_Position = projection * view * vec4(worldPos, 1.0);
}