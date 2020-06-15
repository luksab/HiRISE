#pragma once

#include <vector>

#include "common.hpp"
#include "animation.hpp"

struct pbrObject {
    void render(double currentTime);
    void setup(animated* model, bool tessellation);
    void setup(animated *model, const char* vertex, const char* fragment);
    void setMaticies(glm::mat4* view_mat, glm::mat4* proj_mat);
    void use();
    void setInt(char const * name, int value);
    void setFloat(char const * name, float value);
    void setVec3(char const * name, glm::vec3 value);
    void setMat4(char const * name, glm::mat4* value);

    animated* object;
    unsigned int shaderProgram;
    int model_mat_loc;
    int view_mat_loc;
    glm::mat4* view_matrix;
    int proj_mat_loc;
    glm::mat4* proj_matrix;
    bool useTessellation;
    bool defaultMat;
};
