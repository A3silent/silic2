#version 330 core
// Forest crystal: nearest visible surface, premultiplied surface + emission.
// Six fixed view-ray samples provide stylized internal light, not physical SSS.
// Opaque scene color remains in the framebuffer; no scene-color readback/FBO.
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
out vec4 FragColor;

uniform vec3 cameraPosition;
uniform vec3 brushMinimum;
uniform vec3 brushSize;
uniform vec3 objectColor;
uniform bool useTexture;
uniform sampler2D texture1;
uniform float time;
uniform float phase;
uniform bool depthOnly;

float softLine(vec2 p, vec2 axis, float width) {
    float d2 = dot(p-axis,p-axis);
    return exp(-d2 / max(width*width,0.0001));
}

// The real mesh supplies the outer silhouette. This bounded interior field is
// expressed in its world AABB and intentionally does not replace the mesh shape.
vec2 coreAxis(float h) {
    return vec2(0.035 + 0.045*sin(h*3.8),
                mix(-0.10,0.13,smoothstep(0.25,0.90,h)));
}

vec3 internalField(vec3 q, float aa) {
    float h=clamp(q.y,0.0,1.0);
    vec2 p=q.xz*2.0-1.0;
    vec2 axis=coreAxis(h);
    float width=mix(0.34,0.17,smoothstep(0.05,0.96,h));
    width=max(width,aa*1.5);
    // A broad lit body carries depth; distinct offset paths are sparse beams.
    float core=softLine(p,axis,width);
    float beamWidth=max(mix(0.11,0.062,h),aa*1.2);
    float left=softLine(p,axis+vec2(-0.14-0.12*h,0.10+0.08*h),beamWidth);
    float right=softLine(p,axis+vec2(0.12+0.13*h,-0.08-0.08*h),beamWidth*1.3);
    float back=softLine(p,axis+vec2(0.03-0.06*h,-0.19-0.05*h),beamWidth*1.1);
    float envelope=1.0-smoothstep(0.93,1.04,h);
    float slow=0.92+0.08*sin(time*0.33+h*5.0);
    float mineral=0.78+0.22*sin(h*9.0+p.x*3.3+p.y*2.1);
    return vec3(core*mineral,(left+right*0.8+back*0.6)*0.5,core*0.28)*envelope*slow;
}

float exitDistance(vec3 p,vec3 direction,vec3 minimum,vec3 size) {
    // Avoid 0/0 on exactly horizontal/vertical rays without changing signs.
    vec3 safe=vec3(direction.x<0.0?-1.0:1.0,
                   direction.y<0.0?-1.0:1.0,
                   direction.z<0.0?-1.0:1.0)*max(abs(direction),vec3(0.0001));
    vec3 a=(minimum-p)/safe;
    vec3 b=(minimum+size-p)/safe;
    vec3 farSide=max(a,b);
    return clamp(min(farSide.x,min(farSide.y,farSide.z)),0.05,32.0);
}

void main() {
    // Must not discard or write gl_FragDepth: the color pass uses depth EQUAL.
    if(depthOnly) { FragColor=vec4(0.0); return; }
    vec3 size=max(brushSize,vec3(0.01));
    vec3 q=(FragPos-brushMinimum)/size;
    vec3 toCamera=normalize(cameraPosition-FragPos);
    vec3 ray=-toCamera;
    vec3 n=normalize(Normal);
    if(dot(n,toCamera)<0.0) n=-n;
    float facing=clamp(dot(n,toCamera),0.0,1.0);
    float fresnel=pow(1.0-facing,2.65);
    float aa=max(length(fwidth(q.xz))*2.0,0.012);
    float lengthInBounds=exitDistance(FragPos,ray,brushMinimum,size);
    // The generated main peak is much narrower than its basal-cluster AABB.
    // Do not dilute its core over long empty stretches behind the upper peak.
    float localWidth=mix(13.0,4.8,smoothstep(0.12,0.85,q.y));
    lengthInBounds=min(lengthInBounds,localWidth/max(length(ray.xz),0.25));

    vec3 integral=vec3(0.0);
    float transmission=1.0;
    float stepLength=lengthInBounds/6.0;
    for(int i=0;i<6;++i) {
        vec3 point=FragPos+ray*(float(i)+0.5)*stepLength;
        vec3 local=(point-brushMinimum)/size;
        vec3 field=internalField(local,aa);
        integral+=field*transmission/6.0;
        // Only attenuate the internal integration; framebuffer background is
        // transmitted by the final scalar alpha, not falsely sampled here.
        transmission*=exp(-stepLength*(0.035+0.025*field.x));
    }
    vec3 authored=objectColor;
    if(useTexture) authored*=texture(texture1,TexCoord).rgb;
    float stone=clamp(dot(authored,vec3(0.2126,0.7152,0.0722)),0.0,1.0);
    float etched=smoothstep(0.22,0.52,stone);
    // Broad normal-defined planes survive at 320x200. Fixed material palette
    // approximates the cold sky reflection; the interior supplies emission.
    float facet=0.45+0.30*abs(n.x)+0.25*abs(n.z);
    vec3 tint=mix(vec3(0.13,0.18,0.36),vec3(0.21,0.13,0.35),clamp(phase,0.0,1.0));
    vec3 surface=tint*(0.60+0.55*facet)+authored*0.12;
    vec3 coreColor=mix(vec3(0.20,0.38,0.78),vec3(0.40,0.24,0.72),phase);
    vec3 emission=coreColor*integral.x*1.85;
    emission+=vec3(0.40,0.52,0.88)*integral.y*2.1;
    emission+=vec3(0.12,0.20,0.42)*integral.z;
    emission+=vec3(0.055,0.085,0.15)*fresnel;
    surface+=vec3(0.18,0.28,0.48)*fresnel;
    surface+=vec3(0.20,0.25,0.42)*etched*0.10;
    // Compress only generated radiance, preserving blue/violet saturation and
    // headroom. Opacity never suppresses the independent emitted contribution.
    emission=emission/(vec3(1.0)+emission*0.65);
    emission=min(emission,vec3(0.53,0.64,0.82));
    float absorption=1.0-transmission;
    // Denser mineral body; retain limited transmission and independent emission.
    float alpha=clamp(0.80+0.14*absorption+0.17*fresnel+0.055*integral.x,0.80,0.95);
    FragColor=vec4(surface*alpha+emission,alpha);
}
