#pragma once

#include <vector>

#include "common.hpp"
#include "bones.hpp"

struct boneObject {
    void render(double currentTime);
    void render(double currentTime, unsigned int shaderProg);
    void setup(bones* model, bool tessellation);
    void setup(bones *model, const char* vertex, const char* fragment);
    void setMaticies(glm::mat4* view_mat, glm::mat4* proj_mat);
    void use();
    void setInt(char const * name, int value);
    void setFloat(char const * name, float value);
    void setVec3(char const * name, glm::vec3 value);
    void setMat4(char const * name, glm::mat4* value);
    void scale(float scale);
    void move(float x, float y, float z);

    bones* object;
    glm::mat4 objMat;
    unsigned int shaderProgram;
    unsigned int boneMats;
    int model_mat_loc;
    int view_mat_loc;
    glm::mat4* view_matrix;
    int proj_mat_loc;
    glm::mat4* proj_matrix;
    bool useTessellation;
    bool defaultMat;
};
