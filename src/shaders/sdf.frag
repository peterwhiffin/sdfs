#version 460 core

// enum shape_type {
// 	CIRCLE = 1,
// 	BOX,
// 	TRIANGLE,
// };

const uint CIRCLE = 1;
const uint BOX = 2;
const uint TRIANGLE = 3;

struct entity {
        vec2 position;
        vec4 dim;
        vec4 color;
        uint type;
};

layout(location = 2) in vec2 tex_coord;

layout(location = 5) uniform float time;
layout(location = 6) uniform vec2 mouse_pos;
layout(location = 7) uniform vec2 resolution;
layout(location = 8) uniform vec2 cam_pos;
layout(location = 9) uniform int num_entities;

layout(location = 0) out vec4 frag_color;

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

float sdBox(in vec2 p, in vec2 b)
{
        vec2 d = abs(p) - b;
        return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
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

float opSmoothUnion(float a, float b, float k)
{
        k *= 4.0;
        float h = max(k - abs(a - b), 0.0);
        return min(a, b) - h * h * 0.25 / k;
}
float opSmoothSubtraction(float a, float b, float k)
{
        return -opSmoothUnion(a, -b, k);

        // k *= 4.0;
        // float h = max(k-abs(-a-b),0.0);
        // return max(-a, b) + h*h*0.25/k;
}

float opSmoothIntersection(float a, float b, float k)
{
        return -opSmoothUnion(-a, -b, k);

        // k *= 4.0;
        // float h = max(k-abs(a-b),0.0);
        // return max(a, b) + h*h*0.25/k;
}

float sdSineWave(vec2 p, float freq, float thick, float phase) {
        // vec2 st = gl_FragCoord.xy / vec2(800, 600);
        float y = sin(p.x * freq + phase) * 0.1;
        float line = 1.0 - smoothstep(thick, 0.0, abs(p.y - (y + 0.5)));
        return line;
}

vec4 map() {
        vec2 frag_pos = (2.0 * gl_FragCoord.xy - resolution) / resolution;
        frag_pos *= 10.0;
        frag_pos += cam_pos;

        vec4 m;
        m.a = 1.0;
        m.rgb = vec3(0.4, 0.1, 0.4);

        for (int i = 0; i < num_entities; i++) {
                vec2 pos = frag_pos - entities[i].position;
                float d = sdCircle(pos, 1.0);

                switch (entities[i].type) {
                        case CIRCLE:
                        d = sdCircle(pos, entities[i].dim.x);
                        break;
                        case BOX:
                        d = sdBox(pos, entities[i].dim.xy);
                        break;
                        case TRIANGLE:
                        d = sdEquilateralTriangle(pos, entities[i].dim.x);
                        break;
                }

                float temp = m.a;
                m.a = opSmoothUnion(m.a, d, 0.50);

                if (m.a < temp) {
                        m.rgb = entities[i].color.rgb;
                }
                // m.rgb = mix(m.rgb, entities[i].color.rgb, temp - m.a);
                // m.rgb = smoothstep(m.rgb, entities[i].color.rgb, temp - m.a);
        }

        return m;
}
void main() {
        vec4 m = map();

        float epsilon = 0.06;
        vec3 col = vec3(0.4, 0.1, 0.4);

        if (m.a < -epsilon) {
                // col = vec3(0.0, 0.8, 0.1);
                col = m.rgb;
        } else if (m.a < epsilon) {
                col = vec3(1.0, 1.0, 1.0);
        }

        frag_color = vec4(col, 1.0);
}
