#version 330 core
in vec3 normal, pigment, restPosition;
flat in float pulse, shellFlash;
out vec4 FragColor;
void main() {
    vec3 n = normalize(normal);
    float key = max(dot(n, normalize(vec3(-.35, .85, .45))), 0.);
    float fill = max(dot(n, normalize(vec3(.55, .25, -.72))), 0.);
    // Object-space chips stay attached to the animated shell. Fade grain at
    // distance so a subpixel mark cannot flicker under fisheye resampling.
    vec3 cell = floor(restPosition * 34.);
    float chip = fract(sin(dot(cell, vec3(17.89, 9.13, 4.77))) * 638.13);
    float grain = clamp(1. - max(length(dFdx(restPosition)),length(dFdy(restPosition))) * 38., 0., 1.);
    float patina = 1. - .19 * step(.91, chip) * grain;
    vec3 lit = pigment * patina * (vec3(.48,.48,.56) + key * .55 + fill * vec3(.12,.10,.22));
    lit = mix(lit, vec3(1., .52,.10), pulse);
    FragColor = vec4(mix(lit, vec3(.85,.94,1.), shellFlash), 1.);
}
