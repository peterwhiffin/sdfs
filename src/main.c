#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_timer.h"
#include "cglm/types-struct.h"
#include "stdio.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_video.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_scancode.h>
#include <stddef.h>
#include <stdint.h>
#include "types.h"
#include "glad.c"
#include "renderer.c"

void init_window(struct window *win)
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

	SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
	win->sdl_win = SDL_CreateWindow("SDF", win->width, win->height, flags);
	SDL_GLContext ctx = SDL_GL_CreateContext(win->sdl_win);

	SDL_GL_MakeCurrent(win->sdl_win, ctx);
	SDL_GL_SetSwapInterval(1);
	win->sdl_keys = SDL_GetKeyboardState(NULL);
}

void window_resized(struct window *win)
{
}

void poll_events(struct window *win)
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			win->should_close = true;
			break;
		case SDL_EVENT_QUIT:
			win->should_close = true;
			break;
		case SDL_EVENT_WINDOW_RESIZED:
			win->width = event.window.data1;
			win->height = event.window.data2;
			window_resized(win);
		}
	}
}

void time_update(struct renderer *ren, struct window *win)
{
	uint64_t ticks = SDL_GetTicks();
	float current_time = ticks * 0.001f;
	ren->delta_time = current_time - ren->time;
	ren->time = current_time;
}

void input_update(struct window *win)
{
	SDL_GetMouseState(&win->mouse_pos[0], &win->mouse_pos[1]);
	win->key_r = win->sdl_keys[SDL_SCANCODE_R];
	win->movement = (vec2s){ 0.0, 0.0 };

	win->movement.x += win->sdl_keys[SDL_SCANCODE_A] ? -1 : 0;
	win->movement.x += win->sdl_keys[SDL_SCANCODE_D] ? 1 : 0;
	win->movement.y += win->sdl_keys[SDL_SCANCODE_W] ? 1 : 0;
	win->movement.y += win->sdl_keys[SDL_SCANCODE_S] ? -1 : 0;
}

void update(struct renderer *ren, struct window *win, struct scene *scene)
{
	if (win->key_r) {
		if (win->can_reload_shaders) {
			reload_shaders(ren);
			win->can_reload_shaders = false;
		}
	} else {
		win->can_reload_shaders = true;
	}

	scene->entities[0].transform.pos.x += win->movement.x * scene->cam_speed * ren->delta_time;
	scene->entities[0].transform.pos.y += win->movement.y * scene->cam_speed * ren->delta_time;
	// printf("%f, %f\n", scene->camera.transform.pos.x, scene->camera.transform.pos.y);
}

struct entity *entity_new(struct scene *scene)
{
	struct entity *e = &scene->entities[scene->num_entities];
	e->transform.pos.x = 0;
	e->transform.pos.y = 0;
	e->type = CIRCLE;
	e->dim = (vec4s){ 2.0f, 2.0f, 2.0f, 2.0f };
	e->color = (vec4s){ 0.0, 0.8, 0.1, 1.0 };
	scene->num_entities++;
	return e;
}

vec2s vec2_lerp(vec2s a, vec2s b, float t)
{
	return (vec2s){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

void update_camera(struct scene *scene, struct renderer *ren)
{
	scene->camera.transform.pos =
		vec2_lerp(scene->camera.transform.pos, scene->entities[0].transform.pos, 2.2f * ren->delta_time);
}

int main()
{
	struct renderer ren;
	struct window win;
	struct scene scene;
	scene.max_entities = 1000;
	scene.num_entities = 0;
	scene.entities = malloc(sizeof(*scene.entities) * scene.max_entities);
	scene.camera.transform.pos = (vec2s){ 0.0f, 0.0f };
	scene.cam_speed = 10.0f;
	win.width = 800;
	win.height = 600;
	win.should_close = false;
	win.can_reload_shaders = false;

	init_window(&win);
	init_renderer(&ren, &win, &scene);

	struct entity *e = entity_new(&scene);
	struct entity *e2 = entity_new(&scene);
	struct entity *e3 = entity_new(&scene);
	struct entity *e4 = entity_new(&scene);
	e->transform.pos = (vec2s){ 10.0f, 0.0f };
	e->type = TRIANGLE;
	e->dim.x = 3.0;
	// e->color = (vec4s){ 1.0, 0.0, 0.0, 1.0 };
	e3->transform.pos = (vec2s){ -12.0f, 3.0f };
	e3->type = BOX;
	e3->dim = (vec4s){ 5.0f, 5.0f, 0.0f, 0.0f };
	e4->transform.pos = (vec2s){ 5.0f, -4.0f };

	ren.time = 0.0f;
	ren.delta_time = 0.0f;

	while (!win.should_close) {
		poll_events(&win);
		time_update(&ren, &win);
		input_update(&win);
		update(&ren, &win, &scene);
		update_camera(&scene, &ren);
		draw(&ren, &win, &scene);
		SDL_GL_SwapWindow(win.sdl_win);
	}

	return 0;
}
