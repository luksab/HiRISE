#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 TexCoord_in;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;
uniform vec3 light_dir;

out vec3 interp_normal;
out vec3 interp_light_dir;
out vec2 TexCoord_FS_in;

void main()
{
    gl_Position = proj_mat * view_mat * model_mat * vec4(position.x, position.y, position.z, 1.0);
    // compute directions towards light as well as surface normal in eye-space
    interp_normal = normalize((transpose(inverse(view_mat * model_mat)) * vec4(normal, 0.0)).xyz);
    interp_light_dir = normalize((view_mat * vec4(light_dir, 0.0)).xyz);
    TexCoord_FS_in = TexCoord_in;
}
