#pragma once

#include <vector>

#include "animation.hpp"
#include "common.hpp"
#include "shaderObject.hpp"

struct pbrObject {
    void render(double currentTime);
    void render(glm::mat4& matrix);
    void render(double currentTime, shaderObject shaderProg);
    void renderRotated(float rotation, float time);
    void setup(animated* model, bool tessellation);
    void setup(animated* model, const char* vertex, const char* fragment);
    void setup(animated* model, const char* vertex, const char* fragment, const char* geometry);
    void setup(animated* model, const char* vertex, const char* tess, const char* tesse, const char* fragment);
    void setup(animated* model, std::string vertex, std::string fragment);
    void setup(animated* model, std::string vertex, std::string fragment, std::string geometry);
    void setup(animated* model, std::string vertex, std::string tess, std::string tesse, std::string fragment);
    void reload();
    void reloadCheck();
    bool checkReload();
    void setMaticies(glm::mat4* view_mat, glm::mat4* proj_mat);
    void use();
    void setInt(char const* name, int value);
    void setFloat(char const* name, float value);
    void setVec3(char const* name, glm::vec3 value);
    void setVec4(char const* name, glm::vec4 value);
    void setVec3(char const* name, float x, float y, float z);
    void setMat4(char const* name, glm::mat4* value);

    animated* object;
    shaderObject shaderProgram;
    operator int() const { return shaderProgram; }
};
