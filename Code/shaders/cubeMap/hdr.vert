//https://learnopengl.com/PBR/IBL/Diffuse-irradiance
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 proj_mat;
uniform mat4 view_mat;

out vec3 localPos;

void main()
{
    localPos = aPos;

    mat4 rotView = mat4(mat3(view_mat)); // remove translation from the view matrix
    vec4 clipPos = proj_mat * rotView * vec4(localPos, 1.0);

    gl_Position = clipPos.xyww;
}