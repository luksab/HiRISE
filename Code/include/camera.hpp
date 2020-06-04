#pragma once

#include "common.hpp"

struct camera {
    camera(GLFWwindow* window);

    virtual ~camera();

    glm::mat4
    view_matrix() const;

    glm::vec3
    position() const;
};
