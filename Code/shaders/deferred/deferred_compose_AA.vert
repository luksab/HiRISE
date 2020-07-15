#version 330 core
layout (location = 0) in vec3 position;

out vec2 interp_uv;
out vec4 posPos;

const float FXAA_SUBPIX_SHIFT = 1.0/4.0;

void main()
{
	gl_Position  = vec4(position.xyz, 1.0);
	interp_uv = clamp(0.5 * (position.xy + 1.0), 0.0, 1.0);
	vec2 rcpFrame = vec2(2.0/1920., 2.0/1080.);
	posPos.xy = 0.5 * (position.xy + 1.0);
  	posPos.zw = 0.5 * (position.xy + 1.0) - 
                  (rcpFrame * (0.5 + FXAA_SUBPIX_SHIFT));
}
