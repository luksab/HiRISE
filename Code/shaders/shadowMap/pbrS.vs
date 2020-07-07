#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec4 Index;
layout(location = 4) in vec4 Weight;

uniform mat4 Bone[51];

uniform mat4 model_mat;

void main()
{
    vec4 newVertex;
    vec4 newNormal;
    int index;
    // --------------------

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

    newVertex = model_mat * BoneTransform * vec4(aPos, 1.0);

    gl_Position = newVertex;
}