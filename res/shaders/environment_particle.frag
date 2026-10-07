#version 330 core
in float brightness;
uniform float phase;
out vec4 FragColor;
void main(){FragColor=vec4(mix(vec3(.04,.13,.16),vec3(.11,.055,.17),phase)*brightness,1.0);}
