#pragma once

#include <../imgui/imgui.h>
#include <../imgui/imgui_impl_opengl3.h>
#include <../imgui/imgui_impl_glfw.h>

void
init_imgui(GLFWwindow* window);

void
imgui_new_frame(int initial_width = 0, int initial_height = 0);

void
imgui_render();

void
cleanup_imgui();
