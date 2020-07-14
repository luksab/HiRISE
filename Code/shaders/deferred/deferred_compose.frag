#version 330 core

in vec2 interp_uv;

out vec4 frag_color;

uniform sampler2D tex;
uniform float currentTime;

const float blurSizeH = 1.0 / 1200.0;
const float blurSizeV = 1.0 / 800.0;
const int samples = 8;

void main()
{
    vec4 tex_value = texture2D(tex, interp_uv);

    if(length(tex_value.rgb) > 2.){
        vec4 sum = vec4(0.0);
        for (int x = -samples; x <= samples; x++)
            for (int y = -samples; y <= samples; y++)
                sum += texture(
                    tex,
                    vec2(interp_uv.x + x * blurSizeH, interp_uv.y + y * blurSizeV)
                ) / (5.*samples*samples);
        frag_color = sum;
    }else{
        float before = 13.361;
        float after = 13.7;
        float toBlack = clamp((currentTime-before)/(after-before),0.,1.);
        frag_color = vec4(tex_value.rgb*(1-toBlack), 1.);
    }    
}
