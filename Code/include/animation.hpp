#pragma once

#include <vector>

#include "common.hpp"

struct animated {
    void bind();

    void release();

    void destroy();

    unsigned int vbo;
    unsigned int ibo;
    unsigned int vao;
    std::vector<glm::mat4> transform;
    unsigned int vertex_count;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> uvCords;
    std::vector<glm::uvec3> faces;
};

std::vector<animated>
loadSceneAnim(const char* filename, bool smooth);

animated
loadMeshAnim(const char* filename, bool smooth);
