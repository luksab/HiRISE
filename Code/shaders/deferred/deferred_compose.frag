#version 330 core

in vec2 interp_uv;

out vec4 frag_color;

uniform sampler2D tex;

const float blurSizeH = 1.0 / 300.0;
const float blurSizeV = 1.0 / 200.0;

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
}
