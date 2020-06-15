#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

void main()
{
    TexCoords = aTexCoords;
    WorldPos = vec3(model_mat * vec4(aPos, 1.0));
    Normal = mat3(model_mat) * aNormal;   

    gl_Position =  proj_mat * view_mat * vec4(WorldPos, 1.0);
}