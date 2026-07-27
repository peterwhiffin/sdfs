#include <stddef.h>
#include <stdlib.h>
#include "cglm/types-struct.h"
#include "stdio.h"
#include "types.h"
#include "renderer.h"

#define SHADER_PATH "../../src/shaders/"

// clang-format off
float quad_verts[16] = {
	-1.0f, -1.0f, 0.0f, 0.0f,
	-1.0f,  1.0f, 0.0f, 1.0f,
	 1.0f, -1.0f, 1.0f, 0.0f,
	 1.0f,  1.0f, 1.0f, 1.0f
};

unsigned int quad_indices[4] = {
	0, 1, 2, 3
};
// clang-format on

void create_fullscreen_vao(struct renderer *ren)
{
	GLuint vao;
	GLuint vbo;
	GLuint ebo;

	glCreateVertexArrays(1, &vao);
	glCreateBuffers(1, &vbo);
	glCreateBuffers(1, &ebo);

	glNamedBufferStorage(vbo, sizeof(quad_verts), quad_verts, 0);
	glNamedBufferStorage(ebo, sizeof(quad_indices), quad_indices, 0);

	glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(float) * 4);
	glVertexArrayElementBuffer(vao, ebo);

	glEnableVertexArrayAttrib(vao, 0);
	glEnableVertexArrayAttrib(vao, 1);

	glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));

	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribBinding(vao, 1, 0);

	ren->quad_mesh.vao = vao;
}

void create_texture(struct texture *tex)
{
	glCreateTextures(GL_TEXTURE_2D, 1, &tex->id);
	glTextureParameteri(tex->id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(tex->id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(tex->id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(tex->id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureStorage2D(tex->id, 1, tex->internal_format, tex->width, tex->height);
}

void create_framebuffer(struct framebuffer *fbo)
{
	GLuint rb;
	GLenum attachments[1] = { GL_COLOR_ATTACHMENT0 };

	glCreateFramebuffers(1, &fbo->id);
	glCreateRenderbuffers(1, &rb);
	glNamedRenderbufferStorage(rb, GL_DEPTH_COMPONENT16, fbo->width, fbo->height);

	create_texture(&fbo->render_tex[0]);
	glNamedFramebufferTexture(fbo->id, GL_COLOR_ATTACHMENT0, fbo->render_tex[0].id, 0);

	glNamedFramebufferDrawBuffers(fbo->id, 1, attachments);
	glNamedFramebufferRenderbuffer(fbo->id, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb);

	if (glCheckNamedFramebufferStatus(fbo->id, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer not complete.\n");
	}
}

void create_scene_framebuffer(struct framebuffer *fbo)
{
	GLuint rb;
	GLenum attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0 + 1 };

	glCreateFramebuffers(1, &fbo->id);
	glCreateRenderbuffers(1, &rb);
	glNamedRenderbufferStorage(rb, GL_DEPTH_COMPONENT16, fbo->width, fbo->height);

	create_texture(&fbo->render_tex[0]);
	create_texture(&fbo->render_tex[1]);
	glNamedFramebufferTexture(fbo->id, GL_COLOR_ATTACHMENT0, fbo->render_tex[0].id, 0);
	glNamedFramebufferTexture(fbo->id, GL_COLOR_ATTACHMENT0 + 1, fbo->render_tex[1].id, 0);

	glNamedFramebufferDrawBuffers(fbo->id, 2, attachments);
	glNamedFramebufferRenderbuffer(fbo->id, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb);

	if (glCheckNamedFramebufferStatus(fbo->id, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer not complete.\n");
	}
}

const char *read_file(char *file_path)
{
	long len;
	FILE *f = fopen(file_path, "rb");
	char *buff;
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	buff = malloc(len + 1);
	fread(buff, sizeof(char), len, f);
	buff[len] = '\0';
	fclose(f);
	return buff;
}

void check_shader_compilation(GLuint shader, const char *name)
{
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success) {
		char buff[1024];
		glGetShaderInfoLog(shader, 1024, NULL, buff);
		printf("ERROR::SHADER_COMPILATION::%s::%s\n", name, buff);
	}
}

void check_program_link(GLuint prog)
{
	GLint success;
	glGetProgramiv(prog, GL_LINK_STATUS, &success);

	if (!success) {
		char buff[1024];
		glGetShaderInfoLog(prog, 1024, NULL, buff);
		printf("ERROR::PROGRAM_LINK::%s\n", buff);
	}
}

GLuint load_shader(const char *vert_file, const char *frag_file)
{
	GLuint vert_shader;
	GLuint frag_shader;
	GLuint prog;

	char vert_path[512];
	char frag_path[512];

	snprintf(vert_path, 512, "%s%s", SHADER_PATH, vert_file);
	snprintf(frag_path, 512, "%s%s", SHADER_PATH, frag_file);

	const char *vert_buff = read_file(vert_path);
	const char *frag_buff = read_file(frag_path);

	vert_shader = glCreateShader(GL_VERTEX_SHADER);
	frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	prog = glCreateProgram();

	glShaderSource(vert_shader, 1, &vert_buff, NULL);
	glShaderSource(frag_shader, 1, &frag_buff, NULL);
	glCompileShader(vert_shader);
	glCompileShader(frag_shader);

	check_shader_compilation(vert_shader, vert_file);
	check_shader_compilation(frag_shader, frag_file);

	glAttachShader(prog, vert_shader);
	glAttachShader(prog, frag_shader);

	glLinkProgram(prog);
	check_program_link(prog);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

	free((void *)vert_buff);
	free((void *)frag_buff);
	return prog;
}

void load_all_shaders(struct renderer *ren)
{
	ren->sdf_shader = load_shader("fullscreen.vert", "sdf.frag");
	ren->lighting_shader = load_shader("fullscreen.vert", "lighting.frag");
	ren->fullscreen_shader = load_shader("fullscreen.vert", "fullscreen.frag");
	ren->post_shader = load_shader("fullscreen.vert", "crt.frag");
}

void reload_shaders(struct renderer *ren)
{
	glDeleteProgram(ren->sdf_shader);
	glDeleteProgram(ren->fullscreen_shader);
	glDeleteProgram(ren->lighting_shader);
	glDeleteProgram(ren->post_shader);
	load_all_shaders(ren);

	glUseProgram(ren->sdf_shader);
	float res[2] = { ren->scene_fbo.width, ren->scene_fbo.height };
	glUniform2fv(7, 1, &res[0]);

	glUseProgram(ren->lighting_shader);
	glUniform2fv(13, 1, &res[0]);
	printf("shaders reloaded\n");
}

void draw(struct renderer *ren, struct window *win, struct scene *scene, struct camera *cam)
{
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(struct sdf_shape) * scene->num_sdfs, scene->sdfs);

	glViewport(0, 0, ren->scene_fbo.width, ren->scene_fbo.height);

	glBindFramebuffer(GL_FRAMEBUFFER, ren->scene_fbo.id);
	glUseProgram(ren->sdf_shader);
	glUniform1fv(5, 1, &ren->time);
	glUniform2fv(8, 1, &cam->transform.pos.x);
	glUniform1i(9, scene->num_sdfs);
	glUniform1fv(10, 1, &cam->size);
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, (void *)0);

	glBindFramebuffer(GL_FRAMEBUFFER, ren->lighting_fbo.id);
	glUseProgram(ren->lighting_shader);
	glBindTextureUnit(0, ren->scene_fbo.render_tex[0].id);
	glBindTextureUnit(1, ren->scene_fbo.render_tex[1].id);
	glUniform1ui(11, ren->ray_count);
	glUniform1ui(12, ren->max_steps);
	glUniform1ui(14, ren->use_noise);
	glUniform1fv(16, 1, &ren->constant);
	glUniform1fv(17, 1, &ren->linear);
	glUniform1fv(18, 1, &ren->quadratic);
	glUniform1fv(19, 1, &ren->time);
	glUniform1fv(21, 1, &ren->exposure);
	glUniform1fv(22, 1, &ren->ambient);
	glUniform1fv(23, 1, &ren->gamma);
	glUniform1ui(24, ren->tex_mode);
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, (void *)0);

	glViewport(0, 0, ren->post_fbo.width, ren->post_fbo.height);
	glBindFramebuffer(GL_FRAMEBUFFER, ren->post_fbo.id);
	glUseProgram(ren->post_shader);
	vec2s res = (vec2s){ ren->post_fbo.width, ren->post_fbo.height };
	glUniform2fv(5, 1, &res.x);
	glUniform2fv(6, 1, &ren->pixel_size.x);
	glUniform2fv(7, 1, &ren->real_res.x);
	glBindTextureUnit(0, ren->lighting_fbo.render_tex[0].id);
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, (void *)0);

	glViewport(0, 0, win->width, win->height);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void create_sdf_buffer(struct renderer *ren, struct scene *scene)
{
	glCreateBuffers(1, &ren->sdf_buff);
	glNamedBufferStorage(ren->sdf_buff, sizeof(struct sdf_shape) * scene->max_entities, NULL,
			     GL_DYNAMIC_STORAGE_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ren->sdf_buff);
}

void gl_debug_callback(GLenum src, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const *msg,
		       void const *user_param)
{
	char src_str[128];
	switch (src) {
	case GL_DEBUG_SOURCE_API:
		snprintf(src_str, 128, "%s", "API::");
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		snprintf(src_str, 128, "%s", "WINDOW_SYSTEM::");
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		snprintf(src_str, 128, "%s", "SHADER_COMPILER::");
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		snprintf(src_str, 128, "%s", "THIRD_PARTY::");
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		snprintf(src_str, 128, "%s", "APPLICATION::");
		break;
	case GL_DEBUG_SOURCE_OTHER:
		snprintf(src_str, 128, "%s", "OTHER::");
		break;
	}

	char type_str[128];
	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		snprintf(type_str, 128, "%s", "ERROR::");
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		snprintf(type_str, 128, "%s", "DEPRECATED_BEHAVIOR::");
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		snprintf(type_str, 128, "%s", "UNDEFINED_BEHAVIOR::");
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		snprintf(type_str, 128, "%s", "PORTABILITY::");
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		snprintf(type_str, 128, "%s", "PERFORMANCE::");
		break;
	case GL_DEBUG_TYPE_MARKER:
		snprintf(type_str, 128, "%s", "MARKER::");
		break;
	case GL_DEBUG_TYPE_OTHER:
		snprintf(type_str, 128, "%s", "OTHER::");
		break;
	}

	char severity_str[128];
	switch (severity) {
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		return;
		// snprintf(severity_str, 128, "%s", "NOTIFICATION::");
		break;
	case GL_DEBUG_SEVERITY_LOW:
		snprintf(severity_str, 128, "%s", "LOW::");
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		snprintf(severity_str, 128, "%s", "MEDIUM::");
		break;
	case GL_DEBUG_SEVERITY_HIGH:
		snprintf(severity_str, 128, "%s", "HIGH::");
		break;
	}

	printf("%s%s%s%s\n", src_str, type_str, severity_str, msg);
}

void init_renderer(struct renderer *ren, struct window *win, struct scene *scene)
{
	gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(gl_debug_callback, NULL);
	glDisable(GL_DEPTH_TEST);
	// vec2s res = { 1920.0f, 1080.0f };
	// vec2s res = { 64.0f * 3, 64.0f * 3 };
	vec2s res = { 512.0f * 3.0f, 512.0f * 3.0f };
	// vec2s res = { 128.0f * 3, 128.0f * 3 };

	ren->clear_color[0] = 0.0f;
	ren->clear_color[1] = 0.0f;
	ren->clear_color[2] = 0.0f;
	ren->clear_color[3] = 0.0f;
	ren->clear_depth = 0.0f;
	ren->ambient = 0.1f;
	ren->gamma = 0.51f;
	ren->constant = 0.873f;
	ren->linear = 0.014f;
	ren->exposure = 2.48f;
	ren->quadratic = 0.02f;
	ren->ray_count = 128;
	ren->max_steps = 32;
	ren->pixel_size.x = 3.0f;
	ren->pixel_size.y = 1.0f;
	ren->use_noise = true;
	ren->tex_mode = TEX_MODE_LIT;

	ren->scene_fbo.width = res.x;
	ren->scene_fbo.height = res.y;
	ren->scene_fbo.aspect = ren->scene_fbo.width / ren->scene_fbo.height;
	ren->scene_fbo.render_tex[0].width = ren->scene_fbo.width;
	ren->scene_fbo.render_tex[0].height = ren->scene_fbo.height;
	ren->scene_fbo.render_tex[0].internal_format = GL_RGBA32F;
	ren->scene_fbo.render_tex[1].width = ren->scene_fbo.width;
	ren->scene_fbo.render_tex[1].height = ren->scene_fbo.height;
	ren->scene_fbo.render_tex[1].internal_format = GL_RGBA32F;

	ren->lighting_fbo.width = res.x;
	ren->lighting_fbo.height = res.y;
	ren->lighting_fbo.aspect = ren->lighting_fbo.width / ren->lighting_fbo.height;
	ren->lighting_fbo.render_tex[0].width = ren->lighting_fbo.width;
	ren->lighting_fbo.render_tex[0].height = ren->lighting_fbo.height;
	ren->lighting_fbo.render_tex[0].internal_format = GL_RGBA32F;

	ren->post_fbo.width = res.x;
	ren->post_fbo.height = res.y;
	ren->post_fbo.aspect = ren->post_fbo.width / ren->post_fbo.height;
	ren->post_fbo.render_tex[0].width = ren->post_fbo.width;
	ren->post_fbo.render_tex[0].height = ren->post_fbo.height;

	ren->post_fbo.render_tex[0].internal_format = GL_RGBA32F;

	create_scene_framebuffer(&ren->scene_fbo);
	create_framebuffer(&ren->lighting_fbo);
	create_framebuffer(&ren->post_fbo);
	create_fullscreen_vao(ren);
	create_sdf_buffer(ren, scene);
	load_all_shaders(ren);
	glUseProgram(ren->sdf_shader);
	glUniform2fv(7, 1, &res.x);
	glUseProgram(ren->lighting_shader);
	glUniform2fv(13, 1, &res.x);

	// glBindTextureUnit(0, ren->scene_fbo.render_tex[0].id);
	// glBindTextureUnit(1, ren->scene_fbo.render_tex[1].id);

	glUseProgram(ren->post_shader);
	// glBindTextureUnit(0, ren->lighting_fbo.render_tex[0].id);
	glBindVertexArray(ren->quad_mesh.vao);
}
