#version 430 core

in vec3 interp_pos;
in vec4 interpc_pos;

out vec4 frag_color;

uniform sampler2D tex;
uniform sampler2D height;

uniform vec3 colorA;
uniform vec3 colorB;
uniform float discardFactor;
uniform float h;

const float eps = 1E-06;
const float pi = 3.14159265359;

float derive(vec2 uv, float h){
    uv -= 0.5*h;
    float a = texture2D(height, uv).r;
    return length(vec2(a-texture2D(height, vec2(uv.x+h,uv.y)).r, a-texture2D(height, vec2(uv.x,uv.y+h)).r))*(1/h);
}
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

vec4 applyFog( in vec4  color,      // original color of the pixel
               in float distance)
{
    float fogAmount = (1.0-exp(-distance));
    vec4  fogColor  = vec4(0.5,0.6,0.7,1.0);
    return mix( color, fogColor, fogAmount );
}

void main() {
    vec2 tc;
    tc = interp_pos.xz/2+0.5;
    
    float a = texture2D(tex, tc).r;

    float d = sqrt(derive(tc,h));//min(2.,derive(tc, h));

    float b = d+0.2-a;

    vec4 cola = hsv2rgb(vec4(colorA, 1.0));
    vec4 colb = hsv2rgb(vec4(colorB, 1.0));
    vec4 colorRamp = mix(cola, colb, b);

    //vec4 colorRamp = mix(vec4(1.0, 0.625, 0.354, 1.0), vec4(0.574, 0.914, 0.663, 1.0), b);

    vec4 colorRampHSV = rgb2hsv(colorRamp);

    float e = colorRampHSV.z*a;

    vec4 col = hsv2rgb(vec4(colorRampHSV.xy,e,colorRampHSV.w));

    //col = applyFog(col,length(interpc_pos.xyz));
    //frag_color = col;
    if(texture2D(tex,(interp_pos.xz*discardFactor)/2+0.5).r == 0.0){
        frag_color = vec4(1,0,0,0);
    }else{
        frag_color = col;
    }
}
