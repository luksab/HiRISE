#version 430 core

in vec2 TexCoord_FS_in;

out vec4 frag_color;
out float gl_FragDepth;
uniform sampler2D tex;
uniform vec4 factors;
uniform vec4 factors2;

uniform float discardFactor;

const float PI = 3.14159265323;

// Simplex 2D noise
//
vec3 permute(vec3 x) { return mod(((x*34.0)+1.0)*x, 289.0); }

float snoise(vec2 v){
    const vec4 C = vec4(0.211324865405187, 0.366025403784439,
            -0.577350269189626, 0.024390243902439);
    vec2 i  = floor(v + dot(v, C.yy) );
    vec2 x0 = v -   i + dot(i, C.xx);
    vec2 i1;
    i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;
    i = mod(i, 289.0);
    vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
    + i.x + vec3(0.0, i1.x, 1.0 ));
    vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy),
        dot(x12.zw,x12.zw)), 0.0);
    m = m*m ;
    m = m*m ;
    vec3 x = 2.0 * fract(p * C.www) - 1.0;
    vec3 h = abs(x) - 0.5;
    vec3 ox = floor(x + 0.5);
    vec3 a0 = x - ox;
    m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
    vec3 g;
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    g.yz = a0.yz * x12.xz + h.yz * x12.yw;
    return 130.0 * dot(m, g);
}

float rand(vec2 c){
	return fract(sin(dot(c.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float noise(vec2 p, float freq ){
    freq = 1/freq;
	vec2 ij = floor(p/freq);
	vec2 xy = mod(p,freq)/freq;
	//xy = 3.*xy*xy-2.*xy*xy*xy;
	xy = .5*(1.-cos(PI*xy));
	float a = rand((ij+vec2(0.,0.)));
	float b = rand((ij+vec2(1.,0.)));
	float c = rand((ij+vec2(0.,1.)));
	float d = rand((ij+vec2(1.,1.)));
	float x1 = mix(a, b, xy.x);
	float x2 = mix(c, d, xy.x);
	return mix(x1, x2, xy.y);
}

float pNoise(vec2 p, int res){
	float persistance = .5;
	float n = 0.;
	float normK = 0.;
	float f = 4.;
	float amp = 1.;
	int iCount = 0;
	for (int i = 0; i<117; i++){
		n+=amp*noise(p, f);
		f*=2.;
		normK+=amp;
		amp*=persistance;
		if (iCount == res) break;
		iCount++;
	}
	float nf = n/normK;
	return nf*nf*nf*nf;
}

vec2 cmpxcjg(in vec2 c) {
	return vec2(c.x, -c.y);
}

vec2 cmpxmul(in vec2 a, in vec2 b) {
	return vec2(a.x * b.x - a.y * b.y, a.y * b.x + a.x * b.y);
}

vec2 cmpxpow(in vec2 c, int p) {
	for (int i = 0; i < p; ++i) {
		c = cmpxmul(c, c);
	}
    return c;
}

vec2 cmpxdiv(in vec2 a, in vec2 b) {
    return cmpxmul(a, cmpxcjg(b));
}

float cmpxmag(in vec2 c) {
    return sqrt(c.x * c.x + c.y * c.y);
}

vec2 rek(vec2 z, vec2 c){
    return cmpxmul(z,z)+c;
}

float fractal(vec2 cords){
    vec2 z = rek(vec2(factors2.x,factors2.y), cords);
    for (int i = 0; i<30; i++){
        z = rek(z, cords);
        if(cmpxmag(z) > 1000.){
            return i/20.;
        }
    }
    return 0.;
}

void main() {
    // float noise = 1-abs(snoise(TexCoord_FS_in*1000.));
    // float noise = 1-abs(pNoise(TexCoord_FS_in*10., 5));
    // float noise1 = pNoise(TexCoord_FS_in*100., 10);
    // float noise2 = pNoise(TexCoord_FS_in*1000., 5);
    // float noise3 = pNoise(TexCoord_FS_in*5., 1);
    // // float noise2 = pNoise(TexCoord_FS_in*10., 10);
    vec3 col = texture2D(tex, TexCoord_FS_in).xyz;
    // // frag_color = vec4((noise * factors2.x + factors2.y) * col, 1.);
    // // col *= noise1;
    // // frag_color = vec4(col , 1.);
    // float noiseRes = -abs((noise1*0.5+0.75)*noise2*(abs(noise3*2)+0.5))*0.2;
    // frag_color = vec4(col+noiseRes*.5, 1.);
    frag_color = vec4(col, 1.);
    gl_FragDepth = col.r == 0.?1:gl_FragCoord.z;
}
