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
layout(location = 21) uniform float exposure;
layout(location = 22) uniform float ambient;
layout(location = 23) uniform float gamma;

layout(location = 0) out vec4 frag_color;

#define TAU 6.28318531

bool out_of_bounds(vec2 uv) {
        return uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0;
}

float rand(vec2 co) {
        return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec4 ray_march() {
        float epsilon = 0.0001;
        float one_over_ray_count = 1.0 / float(ray_count);
        float tau_over_ray_count = TAU * one_over_ray_count;
        float noise = use_noise ? rand(tex_coord) : 0.0;
        float aspect = res.y / res.x;

        vec4 start_color = texture(tex, tex_coord);
        vec4 start_dist = texture(distance_tex, tex_coord);
        vec4 radiance = vec4(0.0);

        if (start_dist.a > 0.1) {
                return start_color * 10.0;
        }

        for (int i = 0; i < ray_count; i++) {
                float angle = tau_over_ray_count * (float(i) + noise);
                float angle_c = cos(angle);
                float angle_s = sin(angle);
                float dist_mod = max(abs(angle_c), abs(angle_s));
                float total_dist = start_dist.r;

                vec2 ray_direction_uv = vec2(angle_c, -angle_s);
                vec2 sample_uv = tex_coord + ray_direction_uv * vec2(start_dist.r * aspect, start_dist.r);
                vec4 sample_dist = start_dist;

                //march out of shadow caster
                if (start_dist.r <= epsilon) {
                        float dist = abs(start_dist.r);
                        sample_uv += ray_direction_uv * vec2(dist * aspect, dist);
                        total_dist = dist;
                        float min_dist = 0.005;

                        vec2 grad = vec2(start_dist.g, start_dist.b);

                        if (dot(ray_direction_uv, grad) > 0.00) {
                                for (int j = 0; j < 32; j++) {
                                        sample_dist = texture(distance_tex, sample_uv);

                                        if (sample_dist.r >= epsilon)
                                                break;

                                        dist = max(abs(sample_dist.r) / dist_mod, min_dist);
                                        total_dist += dist;
                                        sample_uv += ray_direction_uv * vec2(dist * aspect, dist);
                                }
                        }
                }

                //march
                for (int s = 0; s < max_steps; s++) {
                        if (out_of_bounds(sample_uv)) break;

                        sample_dist = texture(distance_tex, sample_uv);

                        if (sample_dist.r < epsilon) {
                                sample_uv += ray_direction_uv * vec2(0.01 * aspect, 0.01);
                                vec4 col = texture(tex, sample_uv);
                                float falloff = 1.0 / (constant + (linear * total_dist) + (quadratic * (total_dist * total_dist)));
                                radiance += col * falloff * sample_dist.a;
                                break;
                        } else if (sample_dist.g < epsilon) {
                                // break;
                        }

                        sample_uv += ray_direction_uv * vec2(sample_dist.r * aspect, sample_dist.r);
                        total_dist += sample_dist.r;
                }
        }

        return ((radiance / float(ray_count))) + start_color * ambient;
        // return ((radiance / float(ray_count)));
}

void main() {
        if (show_dist) {
                vec4 dist = texture(distance_tex, tex_coord);
                frag_color = vec4(dist.rgb, 1.0);
        } else if (show_shadow) {
                float dist = texture(distance_tex, tex_coord).g;
                frag_color = vec4(vec3(dist), 1.0);
        } else {
                vec4 color = ray_march();
                vec3 mapped = vec3(1.0) - exp(-color.rgb * exposure);
                mapped = pow(mapped, vec3(1.0 / gamma));
                frag_color = vec4(mapped, 1.0);
        }
}
