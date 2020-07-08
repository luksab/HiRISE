#version 430 core

in vec2 TexCoord_FS_in;

out vec4 frag_color;
uniform sampler2D tex;

void main() {
    frag_color = texture2D(tex, TexCoord_FS_in);
    gl_FragDepth = frag_color.a==0?1:gl_FragCoord.z;
}
