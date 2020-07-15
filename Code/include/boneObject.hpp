#pragma once

#include <vector>

#include "bones.hpp"
#include "common.hpp"
#include "pbrObject.hpp"

struct boneObject {
    void render(double currentTime);
    void render(double currentTime, pbrObject shaderProg);
    void setup(bones* model, bool tessellation);
    void setup(bones* model, const char* vertex, const char* fragment);
    void setMaticies(glm::mat4* view_mat, glm::mat4* proj_mat);
    void use();
    void reload();
    void reloadCheck();
    bool checkReload();
    void setInt(char const* name, int value);
    void setFloat(char const* name, float value);
    void setVec3(char const* name, glm::vec3 value);
    void setVec4(char const* name, glm::vec4 value);
    void setVec3(char const* name, float x, float y, float z);
    void setMat4(char const* name, glm::mat4& value);
    void scale(float scale);
    void move(float x, float y, float z);

    bones* object;
    glm::mat4 objMat;
    unsigned int shaderProgram;
    char* vertex;
    time_t vertexFileTime;
    char* fragment;
    time_t fragmentFileTime;

    std::map<const char*, int> ints;
    std::map<const char*, float> floats;
    std::map<const char*, glm::vec3> vec3s;
    std::map<const char*, glm::vec4> vec4s;
    std::map<const char*, glm::mat4> mat4s;

    unsigned int boneMats;
    int model_mat_loc;
    int view_mat_loc;
    glm::mat4* view_matrix;
    int proj_mat_loc;
    glm::mat4* proj_matrix;
    bool useTessellation;
    bool defaultMat;
};
