#pragma once
#include "SDL3/SDL.h"
#include "glad/glad.h"
#include "cglm/types-struct.h"
#include <stdint.h>

enum shape_type {
	CIRCLE = 1,
	BOX,
	TRIANGLE,
};

struct transform {
	vec2s pos;
};

struct entity {
	struct transform transform;
	vec4s dim;
	vec4s color;
	enum shape_type type;
};

struct scene {
	struct entity *entities;
	struct entity camera;
	float cam_speed;
	uint32_t max_entities;
	uint32_t num_entities;
};

struct window {
	SDL_Window *sdl_win;
	const bool *sdl_keys;
	vec2s movement;
	float width;
	float height;
	float mouse_pos[2];
	bool should_close;
	bool key_r;
	bool can_reload_shaders;
};

struct texture {
	GLuint id;
	GLenum format;
	GLenum internal_format;
	float width;
	float height;
};

struct mesh {
	GLuint vao;
};

struct framebuffer {
	struct texture render_tex;
	struct texture depth_tex;
	GLuint id;
	float width;
	float height;
};

struct renderer {
	struct framebuffer fullscreen_fbo;
	struct mesh quad_mesh;
	float clear_color[4];
	float clear_depth;
	float time;
	float delta_time;
	GLuint sdf_shader;
	GLuint fullscreen_shader;
	GLuint sdf_buff;
};
