#version 330 core
in vec3 worldRay;
out vec4 FragColor;
uniform float phase;
uniform float time;
float hash(vec3 p) { return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453); }
void main() {
    vec3 d = normalize(worldRay);
    float horizon = pow(1.0-abs(d.y),4.0);
    vec3 color = mix(vec3(.008,.014,.036),vec3(.022,.008,.046),phase);
    color += horizon * mix(vec3(.025,.034,.057),vec3(.042,.019,.06),phase);
    // Broad world-anchored, layered nebula ribbons; never move with camera position.
    float ribbon = exp(-15.0*abs(d.y-.22-.17*sin(d.x*4.0+d.z*3.0)));
    float cloud = .5+.5*sin(d.x*18.0 + sin(d.z*13.0)*2.0 + d.y*23.0);
    color += ribbon*(.35+.65*cloud)*mix(vec3(.015,.014,.043),vec3(.033,.01,.049),phase);
    // 3D directional cells avoid equirectangular seams and polar star crowding.
    vec3 cell = floor(d*190.0);
    float star = step(.994,hash(cell));
    color += star * (.12+.16*hash(cell+31.0)) * vec3(.73,.83,1.0);
    vec3 moon = normalize(vec3(-.35,.31,-.9));
    float angular = length(d-moon);
    float body = 1.0-smoothstep(.082,.088,angular);
    float lit = clamp(dot(d-moon,normalize(vec3(-.6,.4,.1)))*10.0+.42,.08,.85);
    color = mix(color,mix(vec3(.065,.09,.15),vec3(.13,.065,.18),phase)*lit,body);
    float ring = exp(-abs(angular-.12-phase*.014)*650.0);
    color += ring*mix(vec3(.06,.11,.15),vec3(.15,.07,.17),phase);
    FragColor = vec4(clamp(color,0.0,.36),1.0);
}
