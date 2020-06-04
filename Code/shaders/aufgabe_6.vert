#version 430 core
layout (location = 0) in vec3 position;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

uniform sampler2D height;

//out vec3 interp_pos;

void main()
{
    //float offset = texture2D(height, position.xy/2+0.5).r;
    gl_Position = vec4(position.x,position.y,position.z,1.0);//proj_mat * view_mat * model_mat * vec4(position.x, position.y, position.z+offset, 1.0);
    //interp_pos = position.xyz;//normalize(vec3(position.x,position.y,position.z-offset));
}
