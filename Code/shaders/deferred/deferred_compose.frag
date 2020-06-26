#version 330 core

in vec2 interp_uv;

out vec4 frag_color;

uniform sampler2D tex;

void main()
{
    vec4 tex_value = texture(tex, interp_uv);

    frag_color = tex_value;
}
