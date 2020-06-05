#pragma once

#include "common.hpp"

struct camera_state {
    // mouse state
    int last_x;
    int last_y;
    int drag_start_x;
    int drag_start_y;
    bool left_down;
    bool middle_down;
    bool right_down;
    bool dragging;

    // transform state
    glm::vec3 look_at;
    float phi;
    float theta;
    float radius;
    glm::mat4 view_mat;
};

struct camera {
    camera(GLFWwindow* window);

    virtual ~camera();

    void update();

    camera_state *
    getState() const;

    glm::mat4
    view_matrix() const;

    glm::vec3
    position() const;
};
