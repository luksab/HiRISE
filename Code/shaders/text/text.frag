#version 430 core
in vec2 TexCoord_FS_in;

uniform sampler2D tex;

uniform vec4 color;

out vec4 frag_color;

void main()
{
    frag_color = color;
    //frag_color = vec4(0.25,0.25,0.25,1.);
}
