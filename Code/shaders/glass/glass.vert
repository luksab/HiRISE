#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 TexCoord_in;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

out vec3 interp_pos;
out vec3 interpG_pos;
out vec3 normal_in;
out vec2 TexCoord_out;
out vec3 interp_normal;

void main()
{
    //float offset = texture2D(height, position.xy/2+0.5).r;
    gl_Position = proj_mat * view_mat * model_mat * vec4(position.x, position.y, position.z, 1.0);
    interpG_pos = (view_mat * model_mat * vec4(position.x, position.y, position.z, 1.0)).xyz;
    interp_pos = position.xyz;//normalize(vec3(position.x,position.y,position.z-offset));
    normal_in = normal;
    TexCoord_out = TexCoord_in;
    interp_normal = normalize((transpose(inverse(view_mat * model_mat)) * vec4(normal, 0.0)).xyz);
}
