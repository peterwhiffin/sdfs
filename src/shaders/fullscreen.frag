#version 460 core

layout(location = 2) in vec2 tex_coord;

layout(binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 frag_color;

void main() {
        vec3 tex_color = texture(tex, tex_coord).rgb;
        frag_color = vec4(tex_color, 1.0);
}
