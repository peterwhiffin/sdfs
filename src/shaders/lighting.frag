#version 460 core

layout(location = 2) in vec2 tex_coord;

layout(binding = 0) uniform sampler2D tex;
layout(location = 10) uniform vec3 tone;
layout(location = 11) uniform uint ray_count;
layout(location = 12) uniform uint max_steps;
layout(location = 13) uniform vec2 res;
layout(location = 14) uniform bool use_noise;
layout(location = 15) uniform bool show_dist;

layout(location = 0) out vec4 frag_color;

#define TAU 6.28318531

bool out_of_bounds(vec2 uv) {
        return uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0;
}

float rand(vec2 co) {
        return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec4 ray_march() {
        vec4 light = texture(tex, tex_coord);
        if (light.a > 0.1) {
                return light;
        }

        float one_over_ray_count = 1.0 / float(ray_count);
        float tau_over_ray_count = TAU * one_over_ray_count;

        float noise = use_noise ? rand(tex_coord) : 0.0;
        vec4 radiance = vec4(0.0);
        float epsilon = 0.000;

        for (int i = 0; i < ray_count; i++) {
                float angle = tau_over_ray_count * (float(i) + noise);
                vec2 ray_direction_uv = vec2(cos(angle), -sin(angle)) / res;
                vec2 sample_uv = tex_coord;

                for (int s = 0; s < max_steps; s++) {
                        vec4 sample_light = texture(tex, sample_uv);
                        float dist = sample_light.r;

                        sample_uv += ray_direction_uv * dist;

                        if (out_of_bounds(sample_uv)) break;

                        if (dist < 0.0001 || sample_light.a > 0.1) {
                                radiance += sample_light;
                                break;
                        }
                }
        }

        return radiance / float(ray_count);
}

void main() {
        if (show_dist) {
                frag_color = texture(tex, tex_coord);
                if (frag_color.a > 0.1) {
                        frag_color.rgb = vec3(0.0);
                } else {
                        frag_color.rgb = vec3(frag_color.r / res.x);
                }
                frag_color.a = 1.0;
        }
        else {
                vec4 color = ray_march();
                frag_color = vec4(color.rgb * tone, 1.0);
        }
}
