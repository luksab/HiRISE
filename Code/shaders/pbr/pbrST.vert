#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec4 Index;
layout(location = 4) in vec4 Weight;

uniform sampler2D heightMap;

uniform mat4 Bone[51];

out vec2 TexCoord_CS_in;
out vec3 Normal_CS_in;
out vec3 WorldPos_CS_in;

out vec3 factor;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

void main()
{
    vec4 newVertex;
    vec4 newNormal;
    int index;
    // --------------------
    Normal_CS_in = normalize(aNormal);
    TexCoord_CS_in = aTexCoords;

    //vec4 weight = normalizeSum(vec4(0.25,0.25,1.,0.));
    vec4 weight = Weight;// should be normalized, but just to be sure
    index = int(Index.x);
    mat4 BoneTransform = Bone[index] * Weight[0];
    index = int(Index.y);
    BoneTransform += Bone[index] * Weight[1];
    index = int(Index.z);
    BoneTransform += Bone[index] * Weight[2];
    index = int(Index.z);
    BoneTransform += Bone[index] * Weight[3];

    if (Weight.x == 0.) {
        BoneTransform = mat4(1);
    }

    newVertex = proj_mat * view_mat * model_mat * BoneTransform * vec4(aPos, 1.0);

    newNormal = normalize(BoneTransform * vec4(Normal_CS_in, 0.));

    WorldPos_CS_in = newVertex.xyz;

    gl_Position = newVertex;
    //gl_Position = proj_mat * view_mat * vec4(aPos, 1.0);

    //float offset = (texture2D(heightMap, aTexCoords).r-.5);
    //gl_Position = proj_mat * view_mat * vec4(newVertex.x+offset*newNormal.x, newVertex.y+offset*newNormal.y, newVertex.z+offset*newNormal.z, 1.0);
    //gl_Position = proj_mat * view_mat * vec4(newVertex.xyz, 1.0);
    Normal_CS_in = newNormal.xyz;
    factor = Weight.xyz;
}