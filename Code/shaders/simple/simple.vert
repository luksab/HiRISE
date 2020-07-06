#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 TexCoord_in;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

uniform sampler2D height;

//out vec3 interp_pos;
out vec2 TexCoord_FS_in;

void main()
{
    gl_Position = proj_mat * view_mat * model_mat * vec4(position, 1.0);
    //interp_pos = position.xyz;//normalize(vec3(position.x,position.y,position.z-offset));
    TexCoord_FS_in = vec2(TexCoord_in.x, 1-TexCoord_in.y);
}
