#include "cglm/types-struct.h"
#include "types.h"
#include "renderer.h"
#include "imgui/dcimgui.h"
#include "imgui/dcimgui_impl_sdl3.h"
#include "imgui/dcimgui_impl_opengl3.h"
#include <stdbool.h>
#include <stdint.h>
#include "stdio.h"

float float_min(float a, float b)
{
	return a < b ? a : b;
}

float float_max(float a, float b)
{
	return a > b ? a : b;
}

struct sdf_shape *entity_add_sdf(struct scene *scene, struct entity *entity)
{
	struct sdf_shape *sdf = &scene->sdfs[scene->num_sdfs];
	sdf->transform.pos.x = 0;
	sdf->transform.pos.y = 0;
	sdf->shape_type = CIRCLE;
	sdf->dim = (vec4s){ 2.0f, 2.0f, 2.0f, 2.0f };
	sdf->color = (vec4s){ 0.0, 0.8, 0.1, 1.0 };
	sdf->brightness= 1.0f;
	sdf->edit_type = SMOOTH_UNION;
	sdf->blend = 0.5f;
	sdf->is_light = true;
	entity->sdf = sdf;
	scene->num_sdfs++;
	return sdf;
}

struct entity *entity_get_new(struct scene *scene)
{
	struct entity *e = &scene->entities[scene->num_entities++];
	e->id = scene->next_id++;
	snprintf(e->name, 256, "%s", "NewEntity");
	e->sdf = NULL;
	return e;
}

void draw_scene(struct editor *editor)
{
	ImGui_Begin("Scene", NULL, 0);

	struct framebuffer *scene_fbo = &editor->ren->scene_fbo;
	struct framebuffer *lighting_fbo = &editor->ren->lighting_fbo;
	ImTextureRef tex_ref = { NULL, lighting_fbo->render_tex[0].id };
	ImVec2 available_size = ImGui_GetContentRegionAvail();
	float aspect = editor->ren->scene_fbo.aspect;
	float width = float_max(scene_fbo->render_tex[0].width, available_size.x);
	float height = width / aspect;

	if (height > available_size.y) {
		height = available_size.y;
		width = height * aspect;
	}

	float y_offset = (available_size.y - height) * 0.5f;
	float x_offset = (available_size.x - width) * 0.5f;
	ImVec2 cursor_pos = ImGui_GetCursorPos();
	ImVec2 centered_pos = { cursor_pos.x + x_offset, cursor_pos.y + y_offset };
	ImGui_SetCursorPos(centered_pos);

	ImVec2 image_size = { width, height };
	ImVec2 uv0 = { 0.0f, 1.0f };
	ImVec2 uv1 = { 1.0f, 0.0f };
	ImGui_ImageEx(tex_ref, image_size, uv0, uv1);

	ImGui_End();
}

void draw_hierarchy(struct editor *editor)
{
	ImGui_Begin("Hierarchy", NULL, 0);

	struct scene *scene = editor->scene;

	for (int i = 0; i < scene->num_entities; i++) {
		struct entity *e = &scene->entities[i];
		char label[256];
		snprintf(label, 256, "%s%u", e->name, e->id);
		bool is_selected = e == editor->selected_entity;

		if (ImGui_SelectableEx(label, is_selected, 0, (ImVec2){ 0.0f, 0.0f })) {
			editor->selected_entity = e;
		}
	}

	ImGui_End();
}

void draw_inspector(struct editor *editor)
{
	ImGui_Begin("Inspector", NULL, 0);
	if (editor->selected_entity) {
		struct sdf_shape *sdf = editor->selected_entity->sdf;
		ImGui_DragFloat2Ex("pos", &sdf->transform.pos.x, 0.01f, 0.0f, 0.0f, NULL, 0);
		ImGui_DragFloat4Ex("dim", &sdf->dim.x, 0.01f, 0.0f, 0.0f, NULL, 0);
		ImGui_DragFloatEx("blend", &sdf->blend, 0.01f, 0.0f, 0.0f, NULL, 0);
		ImGui_ColorEdit4("Color", &sdf->color.r, 0);
		ImGui_DragFloatEx("brightness", &sdf->brightness, 0.01f, 0.0f, 0.0f, NULL, 0);
		ImGui_Checkbox("Is Light", &sdf->is_light);
		char shape_label[128];
		switch (sdf->shape_type) {
		case BOX:
			snprintf(shape_label, 128, "%s", "Box");
			break;
		case CIRCLE:
			snprintf(shape_label, 128, "%s", "Circle");
			break;
		case TRIANGLE:
			snprintf(shape_label, 128, "%s", "Triangle");
			break;
		}

		if (ImGui_BeginCombo("Shape", shape_label, 0)) {
			if (ImGui_Selectable("Box"))
				sdf->shape_type = BOX;
			if (ImGui_Selectable("Circle"))
				sdf->shape_type = CIRCLE;
			if (ImGui_Selectable("Triangle"))
				sdf->shape_type = TRIANGLE;

			ImGui_EndCombo();
		}

		char edit_label[128];
		switch (sdf->edit_type) {
		case UNION:
			snprintf(edit_label, 128, "%s", "Union");
			break;
		case SUBTRACTION:
			snprintf(edit_label, 128, "%s", "Subtraction");
			break;
		case INTERSECTION:
			snprintf(edit_label, 128, "%s", "Intersection");
			break;
		case SMOOTH_UNION:
			snprintf(edit_label, 128, "%s", "Smooth Union");
			break;
		case SMOOTH_SUBTRACTION:
			snprintf(edit_label, 128, "%s", "Smooth Subtraction");
			break;
		case SMOOTH_INTERSECTION:
			snprintf(edit_label, 128, "%s", "Smooth Intersection");
			break;
		}

		if (ImGui_BeginCombo("Edit", edit_label, 0)) {
			if (ImGui_Selectable("Union"))
				sdf->edit_type = UNION;
			if (ImGui_Selectable("Subtraction"))
				sdf->edit_type = SUBTRACTION;
			if (ImGui_Selectable("Intersection"))
				sdf->edit_type = INTERSECTION;
			if (ImGui_Selectable("Smooth Union"))
				sdf->edit_type = SMOOTH_UNION;
			if (ImGui_Selectable("Smooth Subtraction"))
				sdf->edit_type = SMOOTH_SUBTRACTION;
			if (ImGui_Selectable("Smooth Intersection"))
				sdf->edit_type = SMOOTH_INTERSECTION;

			ImGui_EndCombo();
		}
	}
	ImGui_End();
}

void draw_debug(struct editor *editor)
{
	ImGui_Begin("Debug", NULL, 0);

	ImGui_Text("Frame time: %fms", editor->ren->delta_time * 1000.0f);
	ImGui_Text("FPS: %f", 1.0f / editor->ren->delta_time);
	ImGui_DragFloatEx("Cam size", &editor->scene_cam.size, 0.01f, 0.0f, 0.0f, NULL, 0);
	ImGui_DragFloatEx("Ambient", &editor->ren->ambient, 0.001f, 0.0f, 0.0f, NULL, 0);
	ImGui_DragFloatEx("Gamma", &editor->ren->gamma, 0.001f, 0.0f, 0.0f, NULL, 0);
	ImGui_DragFloatEx("Constant", &editor->ren->constant, 0.001f, 0.0f, 0.0f, NULL, 0);
	ImGui_DragFloatEx("Linear", &editor->ren->linear, 0.001f, 0.0f, 0.0f, NULL, 0);
	ImGui_DragFloatEx("Quadratic", &editor->ren->quadratic, 0.001f, 0.0f, 0.0f, NULL, 0);
	ImGui_DragFloatEx("Exposure", &editor->ren->exposure, 0.001f, 0.0f, 0.0f, NULL, 0);
	ImGui_DragIntEx("Ray count", &editor->ren->ray_count, 1.0, 0, 1024, NULL, 0);
	ImGui_DragIntEx("Max steps", &editor->ren->max_steps, 1.0, 0, editor->ren->scene_fbo.render_tex[0].width, NULL, 0);
	ImGui_Checkbox("Use noise", &editor->ren->use_noise);
	ImGui_Checkbox("Show distance", &editor->ren->show_dist);
	ImGui_Checkbox("Show shadow blocker", &editor->ren->show_shadow_blocker);

	if (ImGui_Button("Add Entity")) {
		struct entity *e = entity_get_new(editor->scene);
		entity_add_sdf(editor->scene, e);
	}

	ImGui_End();
}

void editor_draw_begin()
{
	cImGui_ImplOpenGL3_NewFrame();
	cImGui_ImplSDL3_NewFrame();
	ImGui_NewFrame();
	ImGui_DockSpaceOverViewportEx(0, ImGui_GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode, NULL);
}

void editor_draw_end()
{
	ImGui_Render();
	cImGui_ImplOpenGL3_RenderDrawData(ImGui_GetDrawData());
	ImGui_EndFrame();
	ImGui_UpdatePlatformWindows();
	ImGui_RenderPlatformWindowsDefault();
}

void editor_draw(struct editor *editor)
{
	editor_draw_begin();
	ImGui_ShowDemoWindow(NULL);
	draw_scene(editor);
	draw_hierarchy(editor);
	draw_inspector(editor);
	draw_debug(editor);
	editor_draw_end();
}

vec2s vec2_lerp(vec2s a, vec2s b, float t)
{
	return (vec2s){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

void editor_update_camera(struct camera *cam, float dt)
{
	cam->transform.pos = vec2_lerp(cam->transform.pos, cam->target.pos, 1.0f);
}

void editor_update(struct editor *editor)
{
	struct window *win = editor->win;

	if (win->key_r) {
		if (win->can_reload_shaders) {
			reload_shaders(editor->ren);
			win->can_reload_shaders = false;
		}
	} else {
		win->can_reload_shaders = true;
	}

	struct transform *cam_target = &editor->scene_cam.target;

	cam_target->pos.x += win->movement.x * editor->cam_speed * editor->ren->delta_time;
	cam_target->pos.y += win->movement.y * editor->cam_speed * editor->ren->delta_time;

	editor_update_camera(&editor->scene_cam, editor->ren->delta_time);
}

void scene_init(struct scene *scene)
{
	for (int i = 0; i < 4; i++) {
		entity_add_sdf(scene, entity_get_new(scene));
	}

	scene->sdfs[0].transform.pos = (vec2s){ 14.31f, 18.48f };
	scene->sdfs[0].dim = (vec4s){ 2.76f, 2.0f, 2.0f, 2.0f };
	scene->sdfs[0].color = (vec4s){ 1.0f, 1.0f, 1.0f, 1.0f };
	scene->sdfs[0].is_light = true;

	scene->sdfs[1].transform.pos = (vec2s){ 4.67f, -4.0f };
	scene->sdfs[1].dim = (vec4s){ 0.9f, 11.8f, 2.0f, 2.0f };
	scene->sdfs[1].color = (vec4s){ 0.0f, 1.0f, 1.0f, 1.0f };
	scene->sdfs[1].is_light = true;
	scene->sdfs[1].shape_type = BOX;
	scene->sdfs[1].edit_type = UNION;

	scene->sdfs[2].transform.pos = (vec2s){ 4.5f, 1.58f };
	scene->sdfs[2].dim = (vec4s){ 0.8f, 14.8f, 2.0f, 2.0f };
	scene->sdfs[2].color = (vec4s){ 0.0f, 0.0f, 1.0f, 1.0f };
	scene->sdfs[2].shape_type = BOX;
	scene->sdfs[2].edit_type = UNION;
	scene->sdfs[2].is_light = false;

	scene->sdfs[3].transform.pos = (vec2s){ -9.43f, 19.9f };
	scene->sdfs[3].dim = (vec4s){ 3.73f, 2.0f, 2.0f, 2.0f };
	scene->sdfs[3].color = (vec4s){ 1.0f, 0.0f, 1.0f, 1.0f };
	scene->sdfs[3].is_light = true;
	scene->sdfs[3].edit_type = UNION;
}

void editor_init(struct renderer *ren, struct scene *scene, struct window *win, struct editor *editor)
{
	editor->scene = scene;
	editor->ren = ren;
	editor->win = win;
	editor->selected_entity = NULL;
	editor->cam_speed = 10.0f;
	editor->scene_cam.size = 15.62f;
	editor->scene_cam.target.pos = (vec2s){ 0.0f, 10.0f };
	editor->scene_cam.transform.pos = (vec2s){ 0.0f, 10.0f };
	scene_init(scene);
	ImGuiContext *ctx = ImGui_CreateContext(NULL);
	ImGui_SetCurrentContext(ctx);
	ImGuiIO *io = ImGui_GetIO();
	io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui_StyleColorsDark(NULL);
	cImGui_ImplSDL3_InitForOpenGL(win->sdl_win, win->gl_ctx);
	cImGui_ImplOpenGL3_Init();
}
