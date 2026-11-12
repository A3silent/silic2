#version 330 core
in vec3 n;
in vec2 texcoord;
in vec3 localPosition;
in vec3 cameraPosition;
uniform sampler2D baseTexture;
uniform bool textured;
uniform vec4 baseColor;
uniform bool emissive;
uniform bool bloomSource;
uniform bool proceduralMaterial;
out vec4 FragColor;
void main(){
    vec3 albedo=baseColor.rgb*(textured?texture(baseTexture,texcoord).rgb:vec3(1));
    vec3 normal=normalize(n);if(dot(normal,-cameraPosition)<0.0)normal=-normal;
    vec3 key=normalize(vec3(-.45,.86,.28));
    float diffuse=max(dot(normal,key),0.0);
    vec3 lit=albedo*(.40+.60*diffuse);
    if(proceduralMaterial&&!emissive){
        vec3 eye=normalize(-cameraPosition);
        float spec=pow(max(dot(normal,normalize(key+eye)),0.0),42.0);
        float rim=pow(1.0-max(dot(normal,eye),0.0),3.0);
        // Cool facet highlights distinguish near-black armor planes. The red
        // bounce is local to the energized receiver, not a global light change.
        float chamber=exp(-abs(localPosition.y+.005)*27.0)*exp(-abs(abs(localPosition.x)-.04)*35.0);
        chamber*=smoothstep(-.46,-.29,localPosition.z)*(1.0-smoothstep(.14,.23,localPosition.z));
        lit=albedo*(.48+.72*diffuse)+vec3(.36,.33,.42)*spec*.55+vec3(.06,.045,.08)*rim;
        lit+=vec3(.19,.003,.012)*chamber;
    }
    // Private target protocol: 0 = empty, .5 = opaque, 1 = opaque glow source.
    // The compositor reconstructs coverage; the reticle never enters bloom.
    FragColor=vec4(emissive?albedo:lit,bloomSource?1.0:.5);
}
