#version 430 core

in vec3 interp_pos;
in vec3 interpG_pos;
in vec3 normal_in;
in vec2 TexCoord_out;
in vec3 interp_normal;

out vec4 frag_color;

uniform samplerCube skybox;

const float eps = 1E-06;
const float pi = 3.14159265359;

void main() {
    vec3 d = interpG_pos;
    vec3 n = interp_normal;//normalize(normal_in);
    frag_color = texture(skybox, reflect(d,n));
    frag_color.w = 0.1;
    //frag_color = vec4(1.,1.,1.,1.);
}
