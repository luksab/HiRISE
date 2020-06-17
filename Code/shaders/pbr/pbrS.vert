#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec4 Index;
layout (location = 4) in vec4 Weight;

uniform sampler2D heightMap;

uniform mat4 Bone[51];

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

out vec3 factor;

uniform mat4 view_mat;
uniform mat4 proj_mat;

uniform float displacementFactor;

vec4 normalizeSum(vec4 vec){
    float sum = vec.x+vec.y+vec.z+vec.w;
    return vec/sum;
}

void main()
{
    vec4 newVertex;
    vec4 newNormal;
    int index;
    // --------------------
    Normal = normalize(aNormal);
    TexCoords = aTexCoords;
    WorldPos = aPos;

    //vec4 weight = normalizeSum(vec4(0.25,0.25,1.,0.));
    vec4 weight = normalizeSum(Weight); // should be normalized, but just to be sure

    index=int(Index.x); // Cast to int
    newVertex = (Bone[index] * vec4(aPos, 1.0)) * weight.x;
    newNormal = (Bone[index] * vec4(Normal, 0.0)) * weight.x;
    index=int(Index.y); //Cast to int
    newVertex = (Bone[index] * vec4(aPos, 1.0)) * weight.y + newVertex;
    newNormal = (Bone[index] * vec4(Normal, 0.0)) * weight.y + newNormal;

    index=int(Index.z); //Cast to int
    newVertex = (Bone[index] * vec4(aPos, 1.0)) * weight.z + newVertex;
    newNormal = (Bone[index] * vec4(Normal, 0.0)) * weight.z + newNormal;
    index=int(Index.w); //Cast to int
    newVertex = (Bone[index] * vec4(aPos, 1.0)) * weight.w + newVertex;
    newNormal = (Bone[index] * vec4(Normal, 0.0)) * weight.w + newNormal;
    gl_Position = proj_mat * view_mat * vec4(newVertex.xyz, 1.0);
    factor = Index.xyz;
}