#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform sampler2D heightMap;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

uniform float displacementFactor;

void main()
{
    TexCoords = aTexCoords;
    WorldPos = vec3(model_mat * vec4(aPos, 1.0));
    Normal = normalize(mat3(model_mat) * aNormal);

    float offset = displacementFactor*(texture2D(heightMap, aTexCoords).r-.5);
    gl_Position = proj_mat * view_mat * model_mat * vec4(WorldPos.x+offset*Normal.x, WorldPos.y+offset*Normal.y, WorldPos.z+offset*Normal.z, 1.0);

    //gl_Position =  proj_mat * view_mat * vec4(WorldPos, 1.0);
}