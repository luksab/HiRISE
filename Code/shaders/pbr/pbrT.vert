#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoord_CS_in;
out vec3 Normal_CS_in;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

void main()
{
    TexCoord_CS_in = aTexCoords;
    Normal_CS_in = normalize(aNormal);
    gl_Position =  vec4(aPos, 1.0);//proj_mat * view_mat * vec4(WorldPos, 1.0);
}