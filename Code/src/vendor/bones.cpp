#include <bones.hpp>

#include "glm/gtx/string_cast.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <buffer.hpp>
#include <config.hpp>
#include <functional>

#include <chrono>

void bones::bind()
{
    glBindVertexArray(vao);
}

void bones::release()
{
    glBindVertexArray(0);
}

void bones::destroy()
{
    release();
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
}

glm::mat4 aiMatrix4ToGlm(aiMatrix4x4& in)
{
    glm::mat4 out = glm::mat4(1);
    for (int k = 0; k < 4; k++) {
        for (int q = 0; q < 4; q++) {
            out[k][q] = in[q][k];
        }
    }
    return out;
}

glm::mat4 aiMatrix4ToGlm(Matrix4f& in)
{
    glm::mat4 out = glm::mat4(1);
    for (int k = 0; k < 4; k++) {
        for (int q = 0; q < 4; q++) {
            out[k][q] = in.m[q][k];
        }
    }
    return out;
}

const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const string NodeName)
{
    for (uint i = 0; i < pAnimation->mNumChannels; i++) {
        const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

        if (string(pNodeAnim->mNodeName.data) == NodeName) {
            return pNodeAnim;
        }
    }

    return NULL;
}

void ReadNodeHeirarchy(uint frame, const aiNode* pNode, const Matrix4f& ParentTransform, bones* m)
{
    string NodeName(pNode->mName.data);

    const aiAnimation* pAnimation = m->Scene->mAnimations[0];

    Matrix4f NodeTransformation(pNode->mTransformation);

    const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);

    if (pNodeAnim) {
        const aiVector3D& Scaling = pNodeAnim->mScalingKeys[frame].mValue;
        Matrix4f ScalingM;
        ScalingM.InitScaleTransform(Scaling.x, Scaling.y, Scaling.z);
        aiQuaternion RotationQ = pNodeAnim->mRotationKeys[frame].mValue;
        Matrix4f RotationM = Matrix4f(RotationQ.GetMatrix());
        aiVector3D Translation = pNodeAnim->mPositionKeys[frame].mValue;
        Matrix4f TranslationM = Matrix4f();
        TranslationM.InitTranslationTransform(Translation.x, Translation.y, Translation.z);
        NodeTransformation = TranslationM * RotationM * ScalingM;
    }

    Matrix4f GlobalTransformation = ParentTransform * NodeTransformation;

    Matrix4f m_GlobalInverseTransform = m->Scene->mRootNode->mTransformation;
    m_GlobalInverseTransform = m_GlobalInverseTransform.Inverse();
    if (m->BoneMapping.find(NodeName) != m->BoneMapping.end()) {
        uint BoneIndex = m->BoneMapping[NodeName];
        m->boneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m->boneInfo[BoneIndex].BoneOffset;
    }

    for (uint i = 0; i < pNode->mNumChildren; i++) {
        ReadNodeHeirarchy(frame, pNode->mChildren[i], GlobalTransformation, m);
    }
}

void BoneTransform(int frame, vector<Matrix4f>& Transforms, bones* m)
{
    Matrix4f Identity;
    Identity.InitIdentity();

    ReadNodeHeirarchy(frame, m->Scene->mRootNode, Identity, m);

    Transforms.resize(m->NumBones);

    for (uint i = 0; i < m->NumBones; i++) {
        Transforms[i] = m->boneInfo[i].FinalTransformation;
    }
}

void applyRootTransform(bones* m)
{
    for (uint i = 0; i < m->boneTransform.size(); i++) {
        std::vector<Matrix4f> Transforms;
        BoneTransform(i, Transforms, m);
        for (uint j = 0; j < m->boneTransform[i].size(); j++) {
            m->boneTransform[i][j] = aiMatrix4ToGlm(Transforms[j]);
        }
    }
}

std::vector<bones>
loadSceneBone(const char* filename, bool smooth)
{
    return loadSceneBone(filename, 1., smooth);
}

std::vector<bones>
loadSceneBone(const char* filename, double scale, bool smooth)
{
    Assimp::Importer importer;
    int process = aiProcess_JoinIdenticalVertices;
    if (smooth) {
        process |= aiProcess_GenSmoothNormals;
    } else {
        process |= aiProcess_GenNormals;
    }
    const aiScene* scene = importer.ReadFile(SHADER_ROOT + "../models/" + filename, process);
    if (scene == nullptr)
        return {};

    std::vector<bones> objects;
    std::function<void(aiNode*, glm::mat4)> traverse;
    traverse = [&](aiNode* node, glm::mat4 t) {
        aiMatrix4x4 aim = node->mTransformation;
        glm::mat4 new_t(
            aim.a1, aim.b1, aim.c1, aim.d1,
            aim.a2, aim.b2, aim.c2, aim.d2,
            aim.a3, aim.b3, aim.c3, aim.d3,
            aim.a4, aim.b4, aim.c4, aim.d4);
        t = new_t * t;

        aiMatrix4x4 local_t;
        if (node->mNumMeshes > 0) {
            cout << "node->mNumMeshes: " << node->mNumMeshes << "\n";
            for (uint32_t w = 0; w < node->mNumMeshes; ++w) {
                aiMesh* mesh = scene->mMeshes[node->mMeshes[w]];
                if (!mesh->HasBones()) {
                    std::cout << filename << " doesn't have bones";
                    assert(0);
                }
                bones m {};
                m.Scene = scene;
                m.aiBones = mesh->mBones;
                m.boneNames.resize(mesh->mNumBones);
                m.Mesh = mesh;

                m.boneWeight.resize(mesh->mNumVertices);
                m.boneIndex.resize(mesh->mNumVertices);
                cout << "The mesh has " << mesh->mNumBones << " bones and " << scene->mAnimations[0]->mNumChannels << " channels.\n";
                for (uint i = 0; i < mesh->mNumBones; i++) {
                    uint BoneIndex = 0;
                    string BoneName(mesh->mBones[i]->mName.data);

                    if (m.BoneMapping.find(BoneName) == m.BoneMapping.end()) {
                        // Allocate an index for a new bone
                        BoneIndex = m.NumBones;
                        m.NumBones++;
                        m.BoneMapping[BoneName] = BoneIndex;
                        m.boneNames[BoneIndex] = BoneName;
                        //cout << BoneName << "\n";
                    } else {
                        cout << BoneName << "\n";
                        BoneIndex = m.BoneMapping[BoneName];
                        m.boneNames[BoneIndex] = BoneName;
                    }

                    for (uint j = 0; j < mesh->mBones[i]->mNumWeights; j++) {
                        // TODO: make sure the highest weights get saved
                        uint VertexID = mesh->mBones[i]->mWeights[j].mVertexId;
                        int k = 0;
                        for (; k < 4; k++) {
                            if (!(m.boneWeight[VertexID][k] > 0)) {
                                m.boneWeight[VertexID][k] = mesh->mBones[i]->mWeights[j].mWeight;
                                m.boneIndex[VertexID][k] = (float)m.BoneMapping[mesh->mBones[i]->mName.C_Str()];
                                //cout << m.boneWeight[VertexID][k] << ", " << m.boneIndex[VertexID][k] << "\n";
                                k = 5;//break
                            }
                        }
                        if (k == 5)// weight not taken into account
                        {
                            cout << "Too many Weights for vertex " << VertexID << "!\n";
                            // TODO : check if weight larger than minimum for vertexID
                        }
                    }
                }
                m.positions.resize(mesh->mNumVertices);
                m.normals.resize(mesh->mNumVertices);
                m.uvCords.resize(mesh->mNumUVComponents[0]);
                m.faces.resize(mesh->mNumFaces);
                m.boneInfo.resize(mesh->mNumBones);

                for (uint i = 0; i < scene->mAnimations[0]->mNumChannels; i++) {
                    string animationName = scene->mAnimations[0]->mChannels[i]->mNodeName.C_Str();
                    m.AimationMapping[animationName] = i;
                }

                //m.timePerFrame = ((1.)/(scene->mAnimations[0]->mTicksPerSecond));
                m.timePerFrame = ((1.) / (24.));

                aiNodeAnim* pNodeAnim = scene->mAnimations[0]->mChannels[node->mMeshes[w]];
                m.boneTransform.resize(pNodeAnim->mNumPositionKeys);
                m.boneFinalTransform.resize(pNodeAnim->mNumPositionKeys);
                //std::vector<std::vector<glm::mat4>>
                for (uint i = 0; i < pNodeAnim->mNumPositionKeys; i++) {
                    m.boneTransform[i].resize(mesh->mNumBones);
                    m.boneFinalTransform[i].resize(mesh->mNumBones);
                    for (uint j = 0; j < mesh->mNumBones; j++) {
                        m.boneTransform[i][j] = glm::mat4(1.);
                        m.boneInfo[j].BoneOffset = mesh->mBones[j]->mOffsetMatrix;
                    }
                }

                uint bone = 0;
                for (uint32_t p = 0; p < scene->mAnimations[0]->mNumChannels; p++) {
                    //m.boneTransform[p].resize(scene->mAnimations[0]->mNumChannels);
                    pNodeAnim = scene->mAnimations[0]->mChannels[p];
                    for (uint i = 0; i < mesh->mNumBones; i++) {
                        if (mesh->mBones[i]->mName == pNodeAnim->mNodeName)
                            bone = i;
                    }

                    glm::mat4 boneMatrix = glm::mat4(1.);

                    for (int k = 0; k < 4; k++) {
                        for (int q = 0; q < 4; q++) {
                            boneMatrix[k][q] = mesh->mBones[bone]->mOffsetMatrix[q][k];
                        }
                    }
                    //cout << "BonePre: " << glm::to_string(boneMatrix) << "\n";

                    for (uint32_t j = 0; j < pNodeAnim->mNumPositionKeys; j++) {
                        // Interpolate scaling and generate scaling transformation matrix
                        const aiVector3D& Scaling = pNodeAnim->mScalingKeys[j].mValue;
                        Matrix4f ScalingM;
                        ScalingM.InitScaleTransform(Scaling.x * scale, Scaling.y * scale, Scaling.z * scale);
                        // printf("Scale\n");
                        // ScalingM.Print();

                        // Interpolate rotation and generate rotation transformation matrix
                        aiQuaternion RotationQ = pNodeAnim->mRotationKeys[j].mValue;
                        Matrix4f RotationM = Matrix4f(RotationQ.GetMatrix());
                        // printf("Rotation\n");
                        // RotationM.Print();

                        // Interpolate translation and generate translation transformation matrix
                        aiVector3D Translation = pNodeAnim->mPositionKeys[j].mValue;
                        Matrix4f TranslationM = Matrix4f();
                        TranslationM.InitTranslationTransform(Translation.x * scale, Translation.y * scale, Translation.z * scale);
                        // printf("Translation\n");
                        // TranslationM.Print();

                        // Combine the above transformations
                        Matrix4f NodeTransformation = TranslationM * RotationM * ScalingM;
                        // printf("Total\n");
                        // NodeTransformation.Print();
                        // printf("\n");
                        // printf("\n");
                        for (int k = 0; k < 4; k++) {
                            for (int q = 0; q < 4; q++) {
                                m.boneTransform[j][bone][k][q] = NodeTransformation.m[q][k];
                                //m.boneTransform[j][bone][k][q] = NodeTransformation.m[k][q];
                            }
                        }
                        //mat4x4((1.000000, 0.000000, 0.000000, 0.000000), (0.000000, 0.000000, -1.000000, 0.000000), (0.000000, 1.000000, 0.000000, 0.000000), (0.000000, 0.000000, 3.923498, 1.000000))
                        //cout << "BonePost: " << glm::to_string(boneMatrix) << "\n";
                        //m.boneTransform[j][bone] = m.boneTransform[j][bone] * boneMatrix;
                    }
                }

                applyRootTransform(&m);

                uint vboSize = 16;
                float* vbo_data = new float[mesh->mNumVertices * vboSize];
                unsigned int* ibo_data = new unsigned int[mesh->mNumFaces * 3];
                for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
                    glm::vec3 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
                    glm::vec3 nrm(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
                    float u;
                    float v;
                    if (mesh->HasTextureCoords(0)) {
                        u = mesh->mTextureCoords[0][i].x;
                        v = mesh->mTextureCoords[0][i].y;
                    } else {
                        u = v = 0;
                    }
                    glm::vec2 uv(u, v);
                    glm::vec4 index(m.boneIndex[i]);
                    glm::vec4 weight(m.boneWeight[i]);

                    m.positions[i] = pos;
                    m.normals[i] = nrm;

                    vbo_data[vboSize * i + 0] = pos[0];
                    vbo_data[vboSize * i + 1] = pos[1];
                    vbo_data[vboSize * i + 2] = pos[2];
                    vbo_data[vboSize * i + 3] = nrm[0];
                    vbo_data[vboSize * i + 4] = nrm[1];
                    vbo_data[vboSize * i + 5] = nrm[2];
                    vbo_data[vboSize * i + 6] = uv[0];
                    vbo_data[vboSize * i + 7] = uv[1];

                    vbo_data[vboSize * i + 8] = index[0];
                    vbo_data[vboSize * i + 9] = index[1];
                    vbo_data[vboSize * i + 10] = index[2];
                    vbo_data[vboSize * i + 11] = index[3];

                    vbo_data[vboSize * i + 12] = weight[0];
                    vbo_data[vboSize * i + 13] = weight[1];
                    vbo_data[vboSize * i + 14] = weight[2];
                    vbo_data[vboSize * i + 15] = weight[3];
                    //printf("%lf %lf\n",uv[0], uv[1]);
                    //vbo_data[10 * i + 9] = uv[2];
                }

                for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
                    glm::uvec3 face(mesh->mFaces[i].mIndices[0],
                        mesh->mFaces[i].mIndices[1],
                        mesh->mFaces[i].mIndices[2]);
                    m.faces[i] = face;
                    ibo_data[i * 3 + 0] = face[0];
                    ibo_data[i * 3 + 1] = face[1];
                    ibo_data[i * 3 + 2] = face[2];
                }

                glGenVertexArrays(1, &m.vao);
                glBindVertexArray(m.vao);
                m.vbo = makeBuffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW, mesh->mNumVertices * vboSize * sizeof(float), vbo_data);
                m.ibo = makeBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, mesh->mNumFaces * 3 * sizeof(unsigned int), ibo_data);
                glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vboSize * sizeof(float), (void*)0);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vboSize * sizeof(float), (void*)(3 * sizeof(float)));
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vboSize * sizeof(float), (void*)(6 * sizeof(float)));
                glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, vboSize * sizeof(float), (void*)(8 * sizeof(float)));
                glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, vboSize * sizeof(float), (void*)(12 * sizeof(float)));
                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glEnableVertexAttribArray(2);
                glEnableVertexAttribArray(3);
                glEnableVertexAttribArray(4);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);
                m.vertex_count = 3 * mesh->mNumFaces;
                objects.push_back(m);

                delete[] vbo_data;
                delete[] ibo_data;
            }
        }

        for (uint32_t i = 0; i < node->mNumChildren; ++i) {
            traverse(node->mChildren[i], t);
        }
    };

    traverse(scene->mRootNode, glm::identity<glm::mat4>());
    return objects;
}

// glm::mat4 bones::matrixAt(double time)
// {
//     uint PositionIndex = (uint)(time / timePerFrame) % transform.size(); //FindPosition(AnimationTime, pNodeAnim);
//     uint NextPositionIndex = (PositionIndex + 1) % transform.size();
//     float DeltaTime = (float)(timePerFrame);
//     float Factor = timePerFrame / DeltaTime;
//     //assert(Factor >= 0.0f && Factor <= 1.0f);
//     glm::mat4 Start = transform[PositionIndex];
//     glm::mat4 End = transform[NextPositionIndex];
//     glm::mat4 Delta = End - Start;
//     glm::mat4 Out = Start + Factor * Delta;
//     return Out;
// }

void bones::setTime(double time)
{
    for (uint i = 0; i < 1; i++) {
        /* code */
    }
}

bones loadMeshBone(const char* filename, bool smooth)
{
    return loadSceneBone(filename, smooth)[0];
}

bones loadMeshBone(const char* filename, double scale, bool smooth)
{
    return loadSceneBone(filename, scale, smooth)[0];
}
