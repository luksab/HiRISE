#pragma once

#include <vector>

#include "animation.hpp"
#include "common.hpp"

struct pbrMultiObject {
    void render(double currentTime);
    void render(glm::mat4& matrix);
    void render(double currentTime, unsigned int shaderProg);
    void renderRotated(float rotation, float time);
    void setup(std::vector<animated> model, bool tessellation);
    void setup(std::vector<animated> model, const char* vertex, const char* fragment);
    void setup(std::vector<animated> model, const char* vertex, const char* fragment, const char* geometry);
    void setup(std::vector<animated> model, const char* vertex, const char* tess, const char* tesse, const char* fragment);
    void setup(std::vector<animated> model, std::string vertex, std::string fragment);
    void setup(std::vector<animated> model, std::string vertex, std::string fragment, std::string geometry);
    void setup(std::vector<animated> model, std::string vertex, std::string tess, std::string tesse, std::string fragment);
    void setMaticies(glm::mat4* view_mat, glm::mat4* proj_mat);
    void use();
    void setInt(char const* name, int value);
    void setFloat(char const* name, float value);
    void setVec3(char const* name, glm::vec3 value);
    void setVec4(char const* name, glm::vec4 value);
    void setVec3(char const* name, float x, float y, float z);
    void setMat4(char const* name, glm::mat4* value);

    std::vector<animated> object;
    unsigned int shaderProgram;
    int model_mat_loc;
    int view_mat_loc;
    glm::mat4* view_matrix;
    int proj_mat_loc;
    glm::mat4* proj_matrix;
    bool useTessellation;
    bool defaultMat;
};
