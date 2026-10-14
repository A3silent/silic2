#version 330 core
in vec3 worldRay;
out vec4 FragColor;
uniform float phase;
uniform float time;
float hash(vec3 p){return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453);}
void main(){
    vec3 d=normalize(worldRay);
    float horizon=pow(1.0-abs(d.y),5.0);
    vec3 color=vec3(.004,.008,.011)+horizon*vec3(.011,.016,.018);
    // Sparse pinholes through a nearly starless field, without colorful nebulae.
    vec3 cell=floor(d*155.0);
    color+=step(.9991,hash(cell))*(.055+.10*hash(cell+31.0))*vec3(.72,.76,.68);
    vec3 mass=normalize(vec3(-.24,.30,-.95));
    float radius=length(d-mass);
    float body=1.0-smoothstep(.235,.24,radius);
    float grain=.65+.35*hash(floor(d*70.0));
    float edgeLight=smoothstep(.15,.24,radius)*max(0.0,-(d.x-mass.x)*3.0+.2);
    vec3 flesh=vec3(.035,.042,.035)*grain+edgeLight*vec3(.10,.11,.085);
    // A second, incompatible lattice appears within the celestial silhouette.
    float cut=abs(sin((d.x+d.y*.38)*54.0+sin(d.y*22.0)*1.2));
    float ribs=smoothstep(.77,.87,cut)*(1.0-smoothstep(.16,.24,radius));
    flesh=mix(flesh,flesh*.24+ribs*vec3(.075,.088,.06),phase);
    color=mix(color,flesh,body);
    // Off-center dark occluder breaks the disk into an incomplete crescent.
    float occlusion=1.0-smoothstep(.207,.211,length(d-normalize(mass+vec3(.07,.025,.015))));
    color=mix(color,vec3(.002,.004,.005),occlusion*body*(1.0-.18*phase));
    float scar=exp(-abs(radius-.245)*550.0)*max(0.0,-(d.x-mass.x)*2.0);
    color+=scar*vec3(.028,.035,.025);
    FragColor=vec4(clamp(color,0.0,.19),1.0);
}
