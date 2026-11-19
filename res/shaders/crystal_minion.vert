#version 330 core
layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 facetNormal;
layout(location = 2) in vec3 albedo;
layout(location = 3) in float joint;
layout(location = 4) in float channel;
layout(location = 5) in vec4 feetAndSinYaw;
layout(location = 6) in vec4 pose;   // cos yaw, stride sine, shutter opening, organ light
layout(location = 7) in float hit;
uniform mat4 projectionView;
uniform int species;
out vec3 normal, pigment, restPosition;
flat out float pulse, shellFlash;

mat2 turn(float angle) {
    float c=cos(angle), s=sin(angle);
    return mat2(c,s,-s,c);
}
void main() {
    vec3 p=vertex, n=facetNormal;
    float side=joint < 1.5 ? -1. : 1.;
    if (species==0) {
        if (joint>.5) {
            vec2 hip=vec2(.66,-.046);
            mat2 step=turn(side*.24*pose.y);
            p.yz=hip+step*(p.yz-hip);
            p.y+=.034*max(side*pose.y,0.);
            n.yz=step*n.yz;
        } else {
            mat2 lean=turn(-.033*pose.y);
            p.xy=vec2(0,.66)+lean*(p.xy-vec2(0,.66));
            p.y+=.022*abs(pose.y);
            n.xy=lean*n.xy;
        }
    } else {
        if (joint>.5) {
            vec2 hinge=vec2(side*.33,.42);
            mat2 open=turn(-side*.85*pose.z);
            p.xy=hinge+open*(p.xy-hinge);
            n.xy=open*n.xy;
        }
        if (channel>.5) {
            // The low-energy amber seed fills the round firing chamber only
            // once the Sentry has locked its shot.
            float dilation=.52+.48*pose.z;
            p.xy=vec2(0,.65)+(p.xy-vec2(0,.65))*dilation;
            n.xy/=dilation;
        }
    }
    mat2 yaw=mat2(pose.x,-feetAndSinYaw.w,feetAndSinYaw.w,pose.x);
    p.xz=yaw*p.xz;
    n.xz=yaw*n.xz;
    normal=n;
    restPosition=vertex;
    pigment=albedo;
    pulse=channel>.5 ? pose.w : 0.;
    shellFlash=hit*.9*(1.-pulse);
    gl_Position=projectionView*vec4(p+feetAndSinYaw.xyz,1.);
}
