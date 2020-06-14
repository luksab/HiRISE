#pragma once

#include <vector>

#include "common.hpp"

struct animated {
    void bind();

    void release();

    void destroy();

    glm::mat4 matrixAt(double);

    unsigned int vbo;
    unsigned int ibo;
    unsigned int vao;
    std::vector<glm::mat4> transform;
    double timePerFrame;
    unsigned int vertex_count;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> uvCords;
    std::vector<glm::uvec3> faces;
};

std::vector<animated>
loadSceneAnim(const char* filename, bool smooth);

std::vector<animated>
loadSceneAnim(const char *filename, double scale, bool smooth);

animated
loadMeshAnim(const char* filename, bool smooth);

animated
loadMeshAnim(const char *filename, double scale, bool smooth);
