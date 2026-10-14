#version 330 core
in float brightness;
uniform float phase;
out vec4 FragColor;
void main(){FragColor=vec4(mix(vec3(.055,.059,.048),vec3(.045,.064,.038),phase)*brightness,1.0);}
