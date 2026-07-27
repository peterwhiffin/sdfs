#pragma once
#include "SDL3/SDL.h"
#include "SDL3/SDL_video.h"
#include "glad/glad.h"
#include "cglm/types-struct.h"
#include <stdint.h>
#include "imgui/dcimgui.h"

enum shape_type { CIRCLE = 1, BOX, TRIANGLE, PARABOLA, NUM_SHAPES };

enum edit_type {
	UNION = 1,
	SUBTRACTION,
	INTERSECTION,
	SMOOTH_UNION,
	SMOOTH_SUBTRACTION,
	SMOOTH_INTERSECTION,
	NUM_EDITS,
};

enum sdf_flags {
	LIGHT = 1 << 0,
	ANNULAR = 1 << 1,
};

enum tex_mode {
	TEX_MODE_LIT = 0,
	TEX_MODE_UNLIT,
	TEX_MODE_DIST,
	TEX_MODE_GRAD,
};

struct transform {
	vec2s pos;
};

struct sdf_shape {
	struct transform transform;
	vec4s dim;
	vec4s color;
	float blend;
	float brightness;
	enum shape_type shape_type;
	enum edit_type edit_type;
	uint32_t flags;
};

struct camera {
	struct transform transform;
	struct transform target;
	float size;
};

struct entity {
	char name[256];
	uint32_t id;
	struct sdf_shape *sdf;
};

struct scene {
	struct sdf_shape *sdfs;
	struct entity *entities;
	struct sdf_shape camera;
	float cam_speed;
	uint32_t max_entities;
	uint32_t num_sdfs;
	uint32_t num_entities;
	uint32_t next_id;
};

struct window {
	SDL_Window *sdl_win;
	SDL_GLContext gl_ctx;
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
	struct texture render_tex[2];
	struct texture depth_tex;
	GLuint id;
	float width;
	float height;
	float aspect;
};

struct renderer {
	struct framebuffer scene_fbo;
	struct framebuffer lighting_fbo;
	struct framebuffer post_fbo;
	struct mesh quad_mesh;
	vec2s real_res;
	float clear_color[4];
	float clear_depth;
	float time;
	float delta_time;
	float ambient;
	float gamma;
	float constant;
	float linear;
	float quadratic;
	float exposure;
	vec2s pixel_size;
	bool use_noise;
	GLuint ray_count;
	GLuint max_steps;
	GLuint sdf_shader;
	GLuint post_shader;
	GLuint lighting_shader;
	GLuint fullscreen_shader;
	GLuint sdf_buff;
	enum tex_mode tex_mode;
};

struct editor {
	struct scene *scene;
	struct renderer *ren;
	struct window *win;
	struct entity *selected_entity;
	struct camera scene_cam;
	float cam_speed;
};
