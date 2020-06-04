#include <camera.hpp>

#ifndef M_PI
#define M_PI 3.14159265359
#endif

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

static camera_state* state;

camera_state* getState(){
    return state;
}

void
update() {
    glm::mat4 trans_radius = glm::identity<glm::mat4>();
    glm::mat4 trans_center = glm::identity<glm::mat4>();
    glm::mat4 rot_theta = glm::identity<glm::mat4>();
    glm::mat4 rot_phi = glm::identity<glm::mat4>();

    trans_radius[3][2] = -state->radius;
    trans_center[3] = glm::vec4(-state->look_at, 1.f);

    rot_theta[1][1] = cosf(state->theta);
    rot_theta[2][1] = sinf(state->theta);
    rot_theta[1][2] = -rot_theta[2][1];
    rot_theta[2][2] = rot_theta[1][1];

    rot_phi[0][0] = cosf(state->phi);
    rot_phi[0][2] = sinf(state->phi);
    rot_phi[2][0] = -rot_phi[0][2];
    rot_phi[2][2] = rot_phi[0][0];

    state->view_mat = trans_radius * rot_theta * rot_phi * trans_center;
}

void
click_left(int /*x*/, int /*y*/) {
}

void
click_right(int /*x*/, int /*y*/) {
}

void
click_middle(int /*x*/, int /*y*/) {
}

void
move(int /*x*/, int /*y*/, int /*dx*/, int /*dy*/) {
}

void
drag_left(int /*x*/, int /*y*/, int /*dx*/, int /*dy*/) {
}

void
drag_right(int /*x*/, int /*y*/, int dx, int dy) {
    state->theta -= 0.01f * dy;
    if (state->theta < -0.5*M_PI) state->theta = -0.5*M_PI;
    if (state->theta >  0.5*M_PI) state->theta =  0.5*M_PI;
    // technically not necessary, but might avoid numeric instability
    while (state->phi < 0.0)       state->phi += 2.0*M_PI;
    while (state->phi >= 2.0*M_PI) state->phi -= 2.0*M_PI;


    state->phi -= 0.01f * dx;
    update();
}

void
drag_middle(int /*x*/, int /*y*/, int dx, int dy) {
    glm::vec3 u(state->view_mat[0][0],
                state->view_mat[1][0],
                state->view_mat[2][0]);
    glm::vec3 v(state->view_mat[0][1],
                state->view_mat[1][1],
                state->view_mat[2][1]);
    state->look_at -= 0.01f * dx * u;
    state->look_at += 0.01f * dy * v;
    update();
}

void
mouse(int button, int action, int) {
    int crt_x = state->last_x;
    int crt_y = state->last_y;
    if (action == GLFW_PRESS) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:    state->drag_start_x = crt_x;
                                            state->drag_start_y = crt_y;
                                            state->left_down = true;
                                            break;
            case GLFW_MOUSE_BUTTON_RIGHT:   state->drag_start_x = crt_x;
                                            state->drag_start_y = crt_y;
                                            state->right_down = true;
                                            break;
            case GLFW_MOUSE_BUTTON_MIDDLE:  state->drag_start_x = crt_x;
                                            state->drag_start_y = crt_y;
                                            state->middle_down = true;
                                            break;
        }
    } else {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            state->left_down = false;
            if (state->dragging) {
                state->dragging = false;
            } else {
                click_left(state->drag_start_x, state->drag_start_y);
            }
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            state->right_down = false;
            if (state->dragging) {
                state->dragging = false;
            } else {
                click_right(state->drag_start_x, state->drag_start_y);
            }
        } else {
            state->middle_down = false;
            if (state->dragging) {
                state->dragging = false;
            } else {
                click_middle(state->drag_start_x, state->drag_start_y);
            }
        }
    }
}

void
motion(int x, int y) {
    if ((state->left_down || state->right_down || state->middle_down) && (abs(x - state->drag_start_x) + abs(y - state->drag_start_y) > 2)) {
        state->dragging = true;
    }

    if (state->dragging) {
        if (state->left_down) drag_left(x, y, x - state->last_x, y - state->last_y);
        if (state->right_down) drag_right(x, y, x - state->last_x, y - state->last_y);
        if (state->middle_down) drag_middle(x, y, x - state->last_x, y - state->last_y);
    } else {
        move(x, y, x - state->last_x, y - state->last_y);
    }

    state->last_x = x;
    state->last_y = y;
}

void
scroll(int delta) {
    state->radius += 0.5f * delta;
    if (state->radius < 0.001f) state->radius = 0.001f;
    update();
}

camera::camera(GLFWwindow* window) {
    state = new camera_state({});

    state->last_x = 0;
    state->last_y = 0;
    state->drag_start_x = 0;
    state->drag_start_y = 0;
    state->left_down = false;
    state->middle_down = false;
    state->right_down = false;
    state->dragging = false;

    state->look_at = glm::vec3(0.f);
    state->phi = 0.f;
    state->theta = 0.f;
    state->radius = 5.f;

    state->view_mat = glm::identity<glm::mat4>();
    update();

    glfwSetMouseButtonCallback(window, [] (GLFWwindow*, int button, int action, int mods) { mouse(button, action, mods); });
    glfwSetCursorPosCallback(window, [] (GLFWwindow*, double x, double y) { motion(static_cast<int>(x), static_cast<int>(y)); });
    glfwSetScrollCallback(window, [] (GLFWwindow*, double , double delta) { scroll(static_cast<int>(-delta)); });
}

camera::~camera() {
    delete [] state;
}

glm::mat4
camera::view_matrix() const {
    return state->view_mat;
}

glm::vec3
camera::position() const {
    glm::mat3 R(state->view_mat);
    glm::vec3 t(state->view_mat[3]);
    return -glm::transpose(R) * t;
}
