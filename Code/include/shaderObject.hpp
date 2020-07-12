#pragma once

#include <vector>
#include <map>

#include "common.hpp"

struct shaderObject {
    void render(uint vertex_count);
    void render(glm::mat4& matrix, uint vertex_count);
    void setup(bool tessellation);
    void setup(const char* vertex, const char* fragment);
    void setup(const char* vertex, const char* fragment, const char* geometry);
    void setup(const char* vertex, const char* tess, const char* tesse, const char* fragment);
    void setup(std::string vertex, std::string fragment);
    void setup(std::string vertex, std::string fragment, std::string geometry);
    void setup(std::string vertex, std::string tess, std::string tesse, std::string fragment);
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
    void setMat4(char const* name, glm::mat4& value);

    char* vertex;
    time_t vertexFileTime;
    char* tess;
    time_t tessFileTime;
    char* tesse;
    time_t tesseFileTime;
    char* fragment;
    time_t fragmentFileTime;
    char* geometry;
    time_t geometryFileTime;

    uint type;// 1=v,f; 2=v,g,f; 3=v,t,f

    std::map<const char*, int> ints;
    std::map<const char*, float> floats;
    std::map<const char*, glm::vec3> vec3s;
    std::map<const char*, glm::vec4> vec4s;
    std::map<const char*, glm::mat4> mat4s;

    unsigned int shaderProgram;
    operator int() const { return shaderProgram; }
    int model_mat_loc;
    int view_mat_loc;
    glm::mat4* view_matrix;
    int proj_mat_loc;
    glm::mat4* proj_matrix;
    bool useTessellation;
    bool defaultMat;
};
