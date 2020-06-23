#pragma once

#include <map>
#include <vector>

#include "common.hpp"
#include "ogldev_math_3d.h"

#define NUM_BONES_PER_VEREX 4// change glm::vec4 to glm::vec[numBones] for all lines with bone

struct BoneInfo {
    Matrix4f BoneOffset;
    Matrix4f FinalTransformation;

    BoneInfo()
    {
        BoneOffset.SetZero();
        FinalTransformation.SetZero();
    }
};

struct bones {
    void bind();

    void release();

    void destroy();

    void setTime(double);

    unsigned int vbo;
    unsigned int ibo;
    unsigned int vao;
    std::vector<std::vector<glm::mat4>> boneTransform;
    std::vector<std::vector<glm::mat4>> boneFinalTransform;
    const aiScene* Scene;
    std::vector<BoneInfo> boneInfo;
    aiMesh* Mesh;
    std::vector<glm::vec4> boneIndex;
    std::vector<glm::vec4> boneWeight;
    std::map<std::string, uint> BoneMapping;    // maps a bone name to its index
    std::vector<std::string> boneNames;
    std::map<std::string, uint> AimationMapping;// maps an animation name to its index
    unsigned int NumBones;
    aiBone** aiBones;
    double timePerFrame;
    unsigned int vertex_count;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> uvCords;
    std::vector<glm::uvec3> faces;
};

std::vector<bones>
loadSceneBone(const char* filename, bool smooth);

std::vector<bones>
loadSceneBone(const char* filename, double scale, bool smooth);

bones loadMeshBone(const char* filename, bool smooth);

bones loadMeshBone(const char* filename, double scale, bool smooth);
