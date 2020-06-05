#version 430 core

in vec3 interp_pos;

out vec4 frag_color;

uniform sampler2D tex;

const float eps = 1E-06;
const float pi = 3.14159265359;

//hsv/rgb from: http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
// All components are in the range [0…1], including hue.
vec4 rgb2hsv(vec4 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec4(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x, c.w);
}

// All components are in the range [0…1], including hue.
vec4 hsv2rgb(vec4 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return vec4(c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y), c.w);
}

void main() {
    vec2 tc;
    tc = interp_pos.xy/2+0.5;
    frag_color = texture2D(tex,tc);//vec4(1.,1.,1.,1.);
}
