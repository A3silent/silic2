#version 330 core
in vec2 uv;
uniform sampler2D layer;
uniform bool localBloom;
uniform vec2 texelSize;
out vec4 FragColor;
float sourceAt(vec2 p){return max(texture(layer,clamp(p,texelSize*.5,vec2(1)-texelSize*.5)).a-.76,0.0)*4.166667;}
void main(){
    vec4 color=texture(layer,uv);
    float coverage=step(.25,color.a);
    float glow=0.0;
    if(localBloom){
        // Bounded 3-pixel neighborhood in the private viewmodel texture. No
        // world/HUD blur, no extra full-screen framebuffer, no reticle bloom.
        const vec2 d[8]=vec2[](vec2(1,0),vec2(-1,0),vec2(0,1),vec2(0,-1),vec2(.707,.707),vec2(-.707,.707),vec2(.707,-.707),vec2(-.707,-.707));
        for(int i=0;i<8;++i){glow+=sourceAt(uv+d[i]*texelSize)*.0675;glow+=sourceAt(uv+d[i]*texelSize*2.5)*.033;}
    }
    vec3 crystalHaze=vec3(.20,.52,.72);
    vec3 haze=crystalHaze*glow;
    if(coverage>.5){FragColor=vec4(min(color.rgb+haze,vec3(1)),1);}
    else {
        // Straight alpha output; scene detail shows through the low-strength halo.
        float alpha=min(glow*.50,.24);
        FragColor=vec4(crystalHaze,alpha);
    }
}
