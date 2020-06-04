#pragma once

#include <vector>

#include "common.hpp"

struct geometry {
    void bind();

    void release();

    void destroy();

    unsigned int vbo;
    unsigned int ibo;
    unsigned int vao;
    glm::mat4 transform;
    unsigned int vertex_count;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec4> colors;
    std::vector<glm::uvec3> faces;
};

std::vector<geometry>
loadScene(const char* filename, bool smooth);

std::vector<geometry>
loadScene(const char* filename, bool smooth, const glm::vec4& color);

geometry
loadMesh(const char* filename, bool smooth);

geometry
loadMesh(const char* filename, bool smooth, const glm::vec4& color);
