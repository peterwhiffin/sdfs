#version 460 core

// enum shape_type {
// 	CIRCLE = 1,
// 	BOX,
// 	TRIANGLE,
// };

// enum edit_type { UNION = 1, SUBTRACTION, INTERSECTION, SMOOTH_UNION, SMOOTH_SUBTRACTION, SMOOTH_INTERSECTION } ;

const uint CIRCLE = 1;
const uint BOX = 2;
const uint TRIANGLE = 3;

const uint UNION = 1;
const uint SUBTRACTION = 2;
const uint INTERSECTION = 3;
const uint SMOOTH_UNION = 4;
const uint SMOOTH_SUBTRACTION = 5;
const uint SMOOTH_INTERSECTION = 6;

// struct sdf_shape {
// 	struct transform transform;
// 	vec4s dim;
// 	vec4s color;
// 	float blend;
// 	enum shape_type shape_type;
// 	enum edit_type edit_type;
// 	bool is_light;
// };

struct entity {
        vec2 position;
        vec4 dim;
        vec4 color;
        float blend;
        float brightness;
        uint shape_type;
        uint edit_type;
        bool is_light;
};

struct sdf {
        vec3 min_dist;
        float min_light_dist;
        uint closest_index;
};

layout(location = 2) in vec2 tex_coord;

layout(location = 5) uniform float time;
layout(location = 6) uniform vec2 mouse_pos;
layout(location = 7) uniform vec2 resolution;
layout(location = 8) uniform vec2 cam_pos;
layout(location = 9) uniform int num_entities;
layout(location = 10) uniform float cam_size;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 distance_field;

layout(binding = 0, std430) readonly buffer ssbo1 {
        entity entities[];
};

float sdEquilateralTriangle(in vec2 p, in float r)
{
        const float k = sqrt(3.0);
        p.x = abs(p.x) - r;
        p.y = p.y + r / k;
        if (p.x + k * p.y > 0.0) p = vec2(p.x - k * p.y, -k * p.x - p.y) / 2.0;
        p.x -= clamp(p.x, -2.0 * r, 0.0);
        return -length(p) * sign(p.y);
}

float cro(in vec2 a, in vec2 b) {
        return a.x * b.y - a.y * b.x;
}

vec3 sdgTriangle(in vec2 p, in vec2 v[3])
{
        float gs = cro(v[0] - v[2], v[1] - v[0]);
        vec4 res;

        {
                vec2 e = v[1] - v[0], w = p - v[0];
                vec2 q = w - e * clamp(dot(w, e) / dot(e, e), 0.0, 1.0);
                float d = dot(q, q), s = gs * cro(w, e);
                res = vec4(d, q, s);
        }
        {
                vec2 e = v[2] - v[1], w = p - v[1];
                vec2 q = w - e * clamp(dot(w, e) / dot(e, e), 0.0, 1.0);
                float d = dot(q, q), s = gs * cro(w, e);
                res = vec4((d < res.x) ? vec3(d, q) : res.xyz,
                                (s > res.w) ? s : res.w);
        }
        {
                vec2 e = v[0] - v[2], w = p - v[2];
                vec2 q = w - e * clamp(dot(w, e) / dot(e, e), 0.0, 1.0);
                float d = dot(q, q), s = gs * cro(w, e);
                res = vec4((d < res.x) ? vec3(d, q) : res.xyz,
                                (s > res.w) ? s : res.w);
        }

        float d = sqrt(res.x) * sign(res.w);
        return vec3(d, res.yz / d);
}

float sdBox(in vec2 p, in vec2 b)
{
        vec2 d = abs(p) - b;
        return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

vec3 sdgBox(in vec2 p, in vec2 b)
{
        vec2 w = abs(p) - b;
        vec2 s = vec2(p.x < 0.0 ? -1 : 1, p.y < 0.0 ? -1 : 1);
        float g = max(w.x, w.y);
        vec2 q = max(w, 0.0);
        float l = length(q);
        return vec3((g > 0.0) ? l : g,
                s * ((g > 0.0) ? q / l : ((w.x > w.y) ? vec2(1, 0) : vec2(0, 1))));
}

float sdSegment(in vec2 p, in vec2 a, in vec2 b)
{
        vec2 pa = p - a, ba = b - a;
        float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
        return length(pa - ba * h);
}

float sdCircle(vec2 p, float r)
{
        return length(p) - r;
}

vec3 sdgCircle(in vec2 p, in float r)
{
        float d = length(p);
        return vec3(d - r, p / d);
}

float opUnion(float a, float b) {
        return min(a, b);
}

float opSubtraction(float a, float b) {
        return max(-a, b);
}

float opIntersection(float a, float b) {
        return max(a, b);
}

float opSmoothUnion(float a, float b, float k)
{
        k *= 4.0;
        float h = max(k - abs(a - b), 0.0);
        return min(a, b) - h * h * 0.25 / k;
}
float opSmoothSubtraction(float a, float b, float k)
{
        return -opSmoothUnion(a, -b, k);
}

float opSmoothIntersection(float a, float b, float k)
{
        return -opSmoothUnion(-a, -b, k);
}

float sineWave(vec2 p, float freq, float thick, float phase) {
        // vec2 st = gl_FragCoord.xy / vec2(800, 600);
        float y = sin(p.x * freq + phase) * 0.1;
        float line = 1.0 - smoothstep(thick, 0.0, abs(p.y - (y + 0.5)));
        return line;
}

vec3 get_distance(uint i, vec2 pos) {
        switch (entities[i].shape_type) {
                case CIRCLE:
                return sdgCircle(pos, entities[i].dim.x);
                case BOX:
                return sdgBox(pos, entities[i].dim.yx);
                case TRIANGLE:
                float r = entities[i].dim.x;
                vec2 ps[3];
                float k = sqrt(3.0);
                ps[0] = vec2(0.0, r / k);
                ps[1] = vec2(r, -r);
                ps[2] = vec2(-r, -r);

                return sdgTriangle(pos, ps);
        }
}

float get_min(uint i, float m, float d) {
        switch (entities[i].edit_type) {
                case UNION:
                return opUnion(d, m);
                case SUBTRACTION:
                return opSubtraction(m, d);
                case INTERSECTION:
                return opIntersection(d, m);
                case SMOOTH_UNION:
                return opSmoothUnion(d, m, entities[i].blend);
                case SMOOTH_SUBTRACTION:
                return opSmoothSubtraction(m, d, entities[i].blend);
                case SMOOTH_INTERSECTION:
                return opSmoothIntersection(m, d, entities[i].blend);
        }
}

sdf map() {
        sdf sdf_info;
        sdf_info.min_dist.x = 20.0;
        sdf_info.min_light_dist = 20.0;
        sdf_info.closest_index = 0;

        vec2 frag_pos = (2.0 * gl_FragCoord.xy - resolution) / resolution.y;

        frag_pos *= cam_size;
        frag_pos += cam_pos;

        for (int i = 0; i < num_entities; i++) {
                vec2 pos = frag_pos - entities[i].position;

                vec3 d = get_distance(i, pos);
                float m = get_min(i, sdf_info.min_dist.x, d.x);

                if (m < sdf_info.min_dist.x) {
                        sdf_info.min_dist.x = m;
                        sdf_info.min_dist.y = d.y;
                        sdf_info.min_dist.z = d.z;
                        sdf_info.closest_index = i;
                        if (entities[i].is_light) {
                                sdf_info.min_light_dist = m;
                        }
                }
        }

        return sdf_info;
}
void main()
{
        sdf sdf_info = map();

        vec4 col = vec4(0.1, 0.2, 0.7, 0.0);
        vec4 dist = vec4(0.0);
        uint i = sdf_info.closest_index;

        if (sdf_info.min_dist.x < 0.01) {
                col = vec4(entities[i].color.rgb * (1.0 - sdf_info.min_dist.x), 1.0) * entities[i].brightness;
                // col = vec4(entities[i].color.rgb, 1.0) * entities[i].brightness;

                if (entities[i].is_light)
                        dist.a = 1.0;
        }

        // distance_field = vec2(sdf_info.min_light_dist, sdf_info.min_dist) * (1.0 / (cam_size * 2.0));
        dist.r = sdf_info.min_dist.x * (1.0 / (cam_size * 2.0));
        dist.g = sdf_info.min_dist.y * (1.0 / (cam_size * 2.0));
        dist.b = sdf_info.min_dist.z * (1.0 / (cam_size * 2.0));
        distance_field = dist;
        frag_color = col;
}
