#version 430
layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

uniform sampler2D tex;
uniform sampler2D height;


// UI variables
const vec3 irradiance = vec3(0.815,1.,1.);
const vec3 colorA = vec3(0.094,0.263,0.636);
const vec3 colorB = vec3(0.162,0.122,0.606);
const float discardFactor = 1.055;
const float h = 3.0;

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

void main() {
    // base pixel colour for image
    vec4 pixel = vec4(0.0, 0.0, 0.0, 1.0);
    // get index in global work group i.e x,y position
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

    ivec2 dims = imageSize(img_output); // fetch image dimensions
    float x = ((float(pixel_coords.x * 2 - dims.x) / dims.x)-0.5)*2;
    float y = ((float(pixel_coords.y * 2 - dims.y) / dims.y)-0.5)*2;
    x = float(pixel_coords.x)/dims.x;
    y = float(pixel_coords.y)/dims.y;
    
    vec2 tc = vec2(x, y);

    
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
    if(texture2D(tex,((((tc*2)-1)*discardFactor)+1)/2).r == 0.0){
        pixel = vec4(0.,0.,0.,0.);
    }else{
        pixel = vec4(col.rgb*irradiance, 1.0);
    }


  
    // output to a specific pixel in the image
    imageStore(img_output, pixel_coords, pixel);
}