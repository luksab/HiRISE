#version 430 core
in vec2 TexCoord_FS_in;

uniform sampler2D tex;

out vec4 frag_color;

void main()
{
    frag_color = texture2D(tex, TexCoord_FS_in);
    //frag_color = vec4(0.25,0.25,0.25,1.);
}
