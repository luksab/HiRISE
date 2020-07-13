#version 330 core

in vec2 interp_uv;

out vec4 frag_color;

uniform sampler2D tex;

uniform vec3 trans;

const float blurSizeH = 1.0 / 300.0;
const float blurSizeV = 1.0 / 200.0;

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
    vec2 z = rek(vec2(0.,0.), cords);
    for (int i = 0; i<500; i++){
        z = rek(z, cords);
        if(cmpxmag(z) > 5000.){
            return i/100.;
        }
    }
    return 0.;
}

void main()
{
    vec4 tex_value = texture2D(tex, interp_uv);

    if(length(tex_value.rgb) > 2.){
        vec4 sum = vec4(0.0);
        for (int x = -4; x <= 4; x++)
            for (int y = -4; y <= 4; y++)
                sum += texture(
                    tex,
                    vec2(interp_uv.x + x * blurSizeH, interp_uv.y + y * blurSizeV)
                ) / 81.0;
        frag_color = sum;
    }else{
        frag_color = vec4(tex_value.rgb, 1.);
    }

    float zoom = trans.z;
    vec2 mov = trans.xy;

    float x = (interp_uv.x-0.5)/zoom-mov.x;
    float y = (interp_uv.y-0.5)/zoom-mov.y;

    float f = fractal(vec2(x,y));

    frag_color = vec4(f, f, f, 1.);
}
