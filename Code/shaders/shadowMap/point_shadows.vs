#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

uniform bool reverse_normals;

void main()
{
    vs_out.FragPos = vec3(model_mat * vec4(aPos, 1.0));
    if(reverse_normals) // a slight hack to make sure the outer large cube displays lighting from the 'inside' instead of the default 'outside'.
        vs_out.Normal = transpose(inverse(mat3(model_mat))) * (-1.0 * aNormal);
    else
        vs_out.Normal = transpose(inverse(mat3(model_mat))) * aNormal;
    vs_out.TexCoords = aTexCoords;
    gl_Position = proj_mat * view_mat * model_mat * vec4(aPos, 1.0);
}