#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model_mat;

void main()
{
    gl_Position = model_mat * vec4(aPos, 1.0);
}