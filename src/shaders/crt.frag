#version 460 core

layout(location = 2) in vec2 tex_coord;

layout(binding = 0) uniform sampler2D tex;
layout(location = 5) uniform vec2 res;
layout(location = 6) uniform vec2 pixel_size;
layout(location = 7) uniform vec2 real_res;

layout(location = 0) out vec4 frag_color;

void main() {
        vec2 uv = tex_coord / 3.0;
        vec4 coord = gl_FragCoord;

        vec3 color = texture(tex, tex_coord).rgb;

        vec2 crop = mod(real_res, 3.0);

        // if (coord.x < crop.x) {
        //         discard;
        // }
        float p_norm = mod(coord.x, pixel_size.x);
        float sub_pix_size_x = pixel_size.x / 3.0;
        float p_norm_y = mod(coord.y, pixel_size.y);

        float r_thresh = sub_pix_size_x;
        float g_thresh = sub_pix_size_x * 2.0;
        float b_thresh = sub_pix_size_x * 3.0;

        float x_fade = 0.4;
        // float x_fade = 0.5;
        if (p_norm < r_thresh) {
                color = vec3(color.r, 0.0, 0.0);
                float x_thresh = sub_pix_size_x * x_fade;
                float dist = abs(p_norm - (sub_pix_size_x * 0.5));
                float m = x_thresh - dist;
                color *= (m / x_thresh);
        } else if (p_norm < g_thresh) {
                color = vec3(0.0, color.g, 0.0);
                float x_thresh = sub_pix_size_x * x_fade;
                float dist = abs((p_norm - (r_thresh)) - sub_pix_size_x * 0.5);
                float m = x_thresh - dist;
                color *= (m / x_thresh);
        } else {
                color = vec3(0.0, 0.0, color.b);
                float x_thresh = sub_pix_size_x * x_fade;
                float dist = abs((p_norm - (g_thresh)) - sub_pix_size_x * 0.5);
                float m = x_thresh - dist;
                color *= (m / x_thresh);
        }

        float y_thres = pixel_size.y * 0.33;
        // float y_thres = pixel_size.y * 0.5;
        float dist = abs(p_norm_y - (pixel_size.y * 0.5));

        if (dist < y_thres) {
                float m = y_thres - dist;
                color *= 1.0 - (m / y_thres);
        }

        frag_color = vec4(color, 1.0);
        // frag_color = vec4(texture(tex, tex_coord).rgb, 1.0);
}
