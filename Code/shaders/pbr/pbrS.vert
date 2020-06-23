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

    //vec4 weight = normalizeSum(vec4(0.25,0.25,1.,0.));
    vec4 weight = Weight; // should be normalized, but just to be sure

    newVertex = vec4(aPos, 1.0) * (1-dot(weight,vec4(1)));
    newNormal = vec4(Normal, 0.0) * (1-dot(weight,vec4(1)));

    index=int(Index.x); // Cast to int
    newVertex = (Bone[index] * vec4(aPos, 1.0)) * weight.x + newVertex;
    newNormal = (Bone[index] * vec4(Normal, 0.0)) * weight.x + newNormal;
    index=int(Index.y); //Cast to int
    newVertex = (Bone[index] * vec4(aPos, 1.0)) * weight.y + newVertex;
    newNormal = (Bone[index] * vec4(Normal, 0.0)) * weight.y + newNormal;

    index=int(Index.z); //Cast to int
    newVertex = (Bone[index] * vec4(aPos, 1.0)) * weight.z + newVertex;
    newNormal = (Bone[index] * vec4(Normal, 0.0)) * weight.z + newNormal;
    index=int(Index.w); //Cast to int
    newVertex = (Bone[index] * vec4(aPos, 1.0)) * weight.w + newVertex;
    newNormal = (Bone[index] * vec4(Normal, 0.0)) * weight.w + newNormal;

    newNormal = normalize(newNormal);

    WorldPos = newVertex.xyz;

    float offset = (texture2D(heightMap, aTexCoords).r-.5);
    gl_Position = proj_mat * view_mat * vec4(newVertex.x+offset*newNormal.x, newVertex.y+offset*newNormal.y, newVertex.z+offset*newNormal.z, 1.0);
    //gl_Position = proj_mat * view_mat * vec4(newVertex.xyz, 1.0);
    Normal = newNormal.xyz;
    factor = Index.xyz;
}