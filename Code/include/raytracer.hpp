#pragma once

#include "common.hpp"
#include "mesh.hpp"

struct ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

struct intersection {
    unsigned int object;
    unsigned int face;
    float lambda;
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

class raytracer {
public:
    typedef bool (*callback_t)(ray const&, intersection*);

public:
    raytracer(int width, int height, glm::mat4 const& proj_matrix, callback_t intersect_triangle);

    void
    trace(glm::mat4 const& view_matrix, const char* filename);

private:
    int width;
    int height;
    glm::mat4 proj_matrix;
    float near_value;
    float far_value;
    callback_t callback;
};
