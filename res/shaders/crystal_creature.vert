#version 330 core
layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 facetNormal;
layout(location = 2) in vec3 albedo;
layout(location = 3) in float joint;
layout(location = 4) in float channel;
uniform mat4 projectionView;
uniform mat4 root;
uniform mat4 pose[32];
uniform float attackLight, hit;
uniform int coreHits;
out vec3 normal, pigment, restPosition;
flat out float pulse, shellFlash;
void main() {
    mat4 transform = root * pose[int(joint)];
    normal = transpose(inverse(mat3(transform))) * facetNormal;
    restPosition = vertex;
    pigment = albedo;
    int organ = int(channel);
    pulse = organ == 1 || (organ >= 2 && organ <= 4 && organ-2 >= coreHits) ? attackLight : 0.;
    shellFlash = organ < 2 ? hit * (1.-pulse) * .9 : 0.;
    gl_Position = projectionView * transform * vec4(vertex, 1.0);
}
