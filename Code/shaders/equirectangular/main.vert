#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 localPos;

uniform mat4 view_mat;
uniform mat4 proj_mat;

void main()
{
    localPos = aPos;  
    vec4 pos = proj_mat * view_mat * vec4(localPos, 1.0);
    gl_Position = pos.xyww;
}