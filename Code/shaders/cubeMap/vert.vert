#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 proj_mat;
uniform mat4 view_mat;

void main()
{
    TexCoords = aPos;
    vec4 pos = proj_mat * view_mat * vec4(aPos, 0.00001);
    gl_Position = pos.xyww;
}