#version 430 core

in vec3 interp_pos;
in vec3 interpG_pos;
in vec3 normal_in;
in vec2 TexCoord_out;
in vec3 interp_normal;

out vec4 frag_color;

uniform samplerCube skybox;
uniform float power;
// uniform float factor;
// uniform float gamma;
uniform vec3 camPos;

const float eps = 1E-06;
const float pi = 3.14159265359;
const float gamma = 1.1;
const float factor = 1.4;

void main() {
    vec3 d = normalize(interpG_pos-camPos);
    vec3 n = normalize(normal_in);//interp_normal;
    float k = pow(length(cross(normalize(d),n)),power)*factor;
    vec3 envColor = texture(skybox, reflect(d,n)).rgb*(1+k*0.5);
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/gamma));
    frag_color = vec4(envColor, k);
    //frag_color = vec4(1.,1.,1.,1.);
}
