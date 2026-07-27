#include "SDL3/SDL_error.h"
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
#include "editor.c"

void init_window(struct window *win)
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
		goto err;

	win->sdl_keys = SDL_GetKeyboardState(NULL);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

	SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
	win->sdl_win = SDL_CreateWindow("SDF", win->width, win->height, flags);
	if (!win->sdl_win)
		goto err;

	win->gl_ctx = SDL_GL_CreateContext(win->sdl_win);
	if (!win->gl_ctx)
		goto err;

	if (!SDL_GL_MakeCurrent(win->sdl_win, win->gl_ctx))
		goto err;

	if (!SDL_GL_SetSwapInterval(1))
		goto err;

	return;
err:
	printf("ERROR::SDL::%s\n", SDL_GetError());
}

void window_resized(struct window *win)
{
}

void poll_events(struct window *win)
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		cImGui_ImplSDL3_ProcessEvent(&event);
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

int main()
{
	struct renderer ren;
	struct window win;
	struct scene scene;
	struct editor editor;

	scene.max_entities = 1000;
	scene.num_sdfs = 0;
	scene.num_entities = 0;
	scene.next_id = 1;
	scene.sdfs = malloc(sizeof(*scene.sdfs) * scene.max_entities);
	scene.entities = malloc(sizeof(*scene.entities) * scene.max_entities);
	scene.camera.transform.pos = (vec2s){ 0.0f, 0.0f };
	scene.cam_speed = 10.0f;
	win.width = 800;
	win.height = 600;
	win.should_close = false;
	win.can_reload_shaders = false;

	init_window(&win);
	init_renderer(&ren, &win, &scene);
	editor_init(&ren, &scene, &win, &editor);

	ren.time = 0.0f;
	ren.delta_time = 0.0f;

	while (!win.should_close) {
		poll_events(&win);
		time_update(&ren, &win);
		input_update(&win);
		draw(&ren, &win, &scene, &editor.scene_cam);
		editor_update(&editor);
		editor_draw(&editor);
		SDL_GL_SwapWindow(win.sdl_win);
	}

	return 0;
}
