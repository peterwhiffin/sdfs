#version 460 core

layout(location = 2) in vec2 tex_coord;

layout(binding = 0) uniform sampler2D tex;
layout(binding = 1) uniform sampler2D distance_tex;

layout(location = 11) uniform uint ray_count;
layout(location = 12) uniform uint max_steps;
layout(location = 13) uniform vec2 res;
layout(location = 14) uniform bool use_noise;
layout(location = 15) uniform bool show_dist;
layout(location = 16) uniform float constant;
layout(location = 17) uniform float linear;
layout(location = 18) uniform float quadratic;
layout(location = 19) uniform float time;
layout(location = 20) uniform bool show_shadow;

layout(location = 0) out vec4 frag_color;

#define TAU 6.28318531

bool out_of_bounds(vec2 uv) {
        return uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0;
}

float rand(vec2 co) {
        return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec4 ray_march() {
        vec4 start_color = texture(tex, tex_coord);
        vec4 start_dist = texture(distance_tex, tex_coord);

        float epsilon = 0.0001;
        float ambient = 0.1;

        if (start_dist.r < 0.0) {
                return start_color;
        }

        float one_over_ray_count = 1.0 / float(ray_count);
        float tau_over_ray_count = TAU * one_over_ray_count;
        float noise = use_noise ? rand(tex_coord) : 0.0;
        vec4 radiance = start_color * ambient;
        float aspect = res.x / res.y;
        float one_over_aspect = 1.0 / aspect;

        for (int i = 0; i < ray_count; i++) {
                float angle = tau_over_ray_count * (float(i) + noise);
                vec2 ray_direction_uv = vec2(cos(angle), -sin(angle));
                float next_dist_light = start_dist.r;
                float next_dist_shadow = start_dist.g;

                if (next_dist_shadow <= 0.00001) {
                        next_dist_shadow = abs(next_dist_shadow - 0.001) + abs(cos(angle)) * 0.01;
                        // next_dist_shadow = abs(next_dist_shadow - 0.001) + 0.002;
                }

                vec2 sample_uv = tex_coord + ray_direction_uv * vec2(next_dist_shadow * one_over_aspect, next_dist_shadow);
                float total_dist = next_dist_shadow;

                for (int s = 0; s < max_steps; s++) {
                        if (next_dist_light < 0.0001) {
                                sample_uv += ray_direction_uv * 0.01;
                                float falloff = 1.0 / (constant + (linear * total_dist) + (quadratic * (total_dist * total_dist)));
                                vec4 col = texture(tex, sample_uv);
                                radiance += (col * 1.0) * falloff;
                                break;
                        } else if (next_dist_shadow < 0.000001) {
                                break;
                        }

                        vec4 distances = texture(distance_tex, sample_uv);
                        next_dist_light = distances.r;
                        next_dist_shadow = distances.g;
                        sample_uv += ray_direction_uv * vec2(next_dist_shadow * one_over_aspect, next_dist_shadow);
                        if (out_of_bounds(sample_uv)) break;

                        total_dist += next_dist_shadow;
                }
        }

        return ((radiance / float(ray_count))) + start_color * ambient;
}

void main() {
        if (show_dist) {
                float dist = texture(distance_tex, tex_coord).r;
                frag_color = vec4(vec3(dist), 1.0);
        } else if (show_shadow) {
                float dist = texture(distance_tex, tex_coord).g;
                frag_color = vec4(vec3(dist), 1.0);
        } else {
                vec4 color = ray_march();
                frag_color = vec4(color.rgb, 1.0);
        }
}
