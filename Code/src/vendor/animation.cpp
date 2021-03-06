#include <animation.hpp>

#include "ogldev_math_3d.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <buffer.hpp>
#include <config.hpp>
#include <functional>

void animated::bind()
{
    glBindVertexArray(vao);
}

void animated::release()
{
    glBindVertexArray(0);
}

void animated::destroy()
{
    release();
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
}

std::vector<animated>
loadSceneAnim(const char* filename, bool smooth)
{
    return loadSceneAnim(filename, 1., smooth);
}

std::vector<animated>
loadSceneAnim(const char* filename, double scale, bool smooth)
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

    //scene->mCameras[0]->GetCameraMatrix();

    std::vector<animated> objects;
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
            for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                animated m {};
                m.positions.resize(mesh->mNumVertices);
                m.normals.resize(mesh->mNumVertices);
                m.uvCords.resize(mesh->mNumUVComponents[0]);
                m.faces.resize(mesh->mNumFaces);

                if (scene->HasAnimations()) {
                    //Look for the right animation
                    aiNodeAnim* pNodeAnim;
                    bool foundAnim = false;
                    for (uint mAnimationsCounter = 0; mAnimationsCounter < scene->mAnimations[0]->mNumChannels; mAnimationsCounter++) {
                        if (scene->mAnimations[0]->mChannels[mAnimationsCounter]->mNodeName == node->mName) {
                            pNodeAnim = scene->mAnimations[0]->mChannels[node->mMeshes[i]];
                            foundAnim = true;
                        }
                    }
                    if (!foundAnim) {
                        printf("Didn't find pp for %s!\nOptions were:\n", mesh->mName.C_Str());
                        for (uint mAnimationsCounter = 0; mAnimationsCounter < scene->mAnimations[0]->mNumChannels; mAnimationsCounter++) {
                            printf("%s\n", scene->mAnimations[0]->mChannels[mAnimationsCounter]->mNodeName.C_Str());
                        }
                        assert(0);
                    }

                    //m.timePerFrame = ((1.)/(scene->mAnimations[0]->mTicksPerSecond));
                    m.timePerFrame = ((1.) / (24.));

                    m.transform.resize(pNodeAnim->mNumPositionKeys);
                    for (uint32_t j = 0; j < pNodeAnim->mNumPositionKeys; j++) {
                        // Interpolate scaling and generate scaling transformation matrix
                        const aiVector3D& Scaling = pNodeAnim->mScalingKeys[j].mValue;
                        Matrix4f ScalingM;
                        ScalingM.InitScaleTransform(Scaling.x * scale, Scaling.y * scale, Scaling.z * scale);

                        // Interpolate rotation and generate rotation transformation matrix
                        aiQuaternion RotationQ = pNodeAnim->mRotationKeys[j].mValue;
                        Matrix4f RotationM = Matrix4f(RotationQ.GetMatrix());

                        // Interpolate translation and generate translation transformation matrix
                        aiVector3D Translation = pNodeAnim->mPositionKeys[j].mValue;
                        Matrix4f TranslationM = Matrix4f();
                        TranslationM.InitTranslationTransform(Translation.x * scale, Translation.y * scale, Translation.z * scale);

                        // Combine the above transformations
                        Matrix4f NodeTransformation = TranslationM * RotationM * ScalingM;
                        m.transform[j] = glm::mat4(1);
                        for (int k = 0; k < 4; k++) {
                            for (int p = 0; p < 4; p++) {
                                m.transform[j][k][p] = NodeTransformation.m[p][k];
                            }
                        }
                    }
                } else {//no animation
                    m.timePerFrame = ((1.) / (24.));
                    m.transform.resize(1);
                    m.transform[0] = glm::mat4(1);
                }

                bool hasTexture = true;
                if (!mesh->HasTextureCoords(0)) {
                    hasTexture = false;
                    printf("%s doesn't have uv map!");
                }
                float* vbo_data = new float[mesh->mNumVertices * 8];
                unsigned int* ibo_data = new unsigned int[mesh->mNumFaces * 3];
                for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
                    glm::vec3 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
                    if (!mesh->HasNormals())
                        return;
                    glm::vec3 nrm(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

                    float u = 0;
                    float v = 0;
                    if (hasTexture) {
                        u = mesh->mTextureCoords[0][i].x;
                        v = mesh->mTextureCoords[0][i].y;
                    }
                    glm::vec2 uv(u, v);

                    m.positions[i] = pos;
                    m.normals[i] = nrm;

                    vbo_data[8 * i + 0] = pos[0];
                    vbo_data[8 * i + 1] = pos[1];
                    vbo_data[8 * i + 2] = pos[2];
                    vbo_data[8 * i + 3] = nrm[0];
                    vbo_data[8 * i + 4] = nrm[1];
                    vbo_data[8 * i + 5] = nrm[2];
                    vbo_data[8 * i + 6] = uv[0];
                    vbo_data[8 * i + 7] = uv[1];
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
                m.vbo = makeBuffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW, mesh->mNumVertices * 8 * sizeof(float), vbo_data);
                m.ibo = makeBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, mesh->mNumFaces * 3 * sizeof(unsigned int), ibo_data);
                glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glEnableVertexAttribArray(2);
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

glm::mat4 animated::matrixAt(double time)
{
    uint PositionIndex = (uint)(time / timePerFrame) % transform.size();//FindPosition(AnimationTime, pNodeAnim);
    uint NextPositionIndex = (PositionIndex + 1) % transform.size();
    float DeltaTime = (float)(timePerFrame);
    float Factor = fmod(time, timePerFrame) / DeltaTime;
    //assert(Factor >= 0.0f && Factor <= 1.0f);
    glm::mat4 Start = transform[PositionIndex];
    glm::mat4 End = transform[NextPositionIndex];
    glm::mat4 Delta = End - Start;
    glm::mat4 Out = Start + Factor * Delta;
    return Out;
}

animated
loadMeshAnim(const char* filename, bool smooth)
{
    return loadSceneAnim(filename, smooth)[0];
}

animated
loadMeshAnim(const char* filename, double scale, bool smooth)
{
    return loadSceneAnim(filename, scale, smooth)[0];
}

animated
toAnimated(geometry input)
{
    animated m {};
    m.vbo = input.vbo;
    m.ibo = input.ibo;
    m.vao = input.vao;
    m.transform.resize(1);
    m.transform[0] = input.transform;
    m.timePerFrame = ((1.) / (24.));
    m.vertex_count = input.vertex_count;
    m.positions = input.positions;
    m.normals = input.normals;
    m.faces = input.faces;

    return m;
}
