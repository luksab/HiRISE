#version 330 core
layout (location = 0) in vec3 position;

out vec2 interp_uv;

void main()
{
	gl_Position  = vec4(position.xyz, 1.0);
	interp_uv = clamp(0.5 * (position.xy + 1.0), 0.0, 1.0);
}
