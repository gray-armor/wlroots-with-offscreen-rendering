#include <stdio.h>
#include <wlr/render/egl.h>

#include "internal.h"

#define REPAINT_INTERVAL 10
#define RENDER_WIDTH 800
#define RENDER_HEIGHT 600

const char* vertex_shader_source =
    "attribute vec4 vPosition;  \n"
    "                           \n"
    "void main()                \n"
    "{                          \n"
    "  gl_Position = vPosition; \n"
    "}                          \n";

const char* fragment_shader_source =
    "void main()                                \n"
    "{                                          \n"
    "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); \n"
    "}                                          \n";

static GLuint
compile_shader(const char* shader_source, GLenum type)
{
  GLuint shader;
  GLint compiled;

  shader = glCreateShader(type);
  if (shader == 0) {
    fprintf(stderr, "Failed to create shader\n");
    return 0;
  }

  glShaderSource(shader, 1, &shader_source, NULL);

  glCompileShader(shader);

  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

  if (!compiled) {
    GLint info_len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
    if (info_len > 1) {
      char* info_log = malloc(sizeof(char) * info_len);

      glGetShaderInfoLog(shader, info_len, NULL, info_log);
      fprintf(stderr, "Error compiling shader:\n%s\n", info_log);
      free(info_log);
    }

    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

void
offscreen_blit(
    struct offscreen* self, GLint x, GLint y, GLsizei width, GLsizei height)
{
  GLfloat vertices[] = {   //
      +0.0f, +0.5f, 0.0f,  //
      -0.5f, -0.5f, 0.0f,  //
      +0.5f, -0.5f, 0.0f};

  UNUSED(self);
  UNUSED(x);
  UNUSED(y);
  UNUSED(width);
  UNUSED(height);

  glUseProgram(self->program);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
  glEnableVertexAttribArray(0);

  glDrawArrays(GL_TRIANGLES, 0, 3);
}

int
offscreen_repaint(void* data)
{
  struct offscreen* self = data;

  wlr_egl_make_current(self->egl);
  glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);
  glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);
  glClearColor(1, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  wl_event_source_timer_update(self->repaint_source, REPAINT_INTERVAL);

  return 1;
}

struct offscreen*
offscreen_create(struct wlr_egl* egl, struct wl_display* display)
{
  struct offscreen* self;
  struct wl_event_loop* loop = wl_display_get_event_loop(display);
  GLenum status;
  GLint linked;

  self = zalloc(sizeof *self);
  if (self == NULL) {
    fprintf(stderr, "No memory\n");
    goto err;
  }

  self->egl = egl;
  self->repaint_source = wl_event_loop_add_timer(loop, offscreen_repaint, self);

  wlr_egl_make_current(egl);

  // Build the texture that will serve as the color attachment for the
  // framebuffer
  glGenTextures(1, &self->color_buffer);
  glBindTexture(GL_TEXTURE_2D, self->color_buffer);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RENDER_WIDTH, RENDER_HEIGHT, 0,
      GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  glBindTexture(GL_TEXTURE_2D, 0);

  // Build the texture that will serve as depth attachment for the framebuffer
  glGenRenderbuffers(1, &self->depth_buffer);
  glBindRenderbuffer(GL_RENDERBUFFER, self->depth_buffer);
  glRenderbufferStorage(
      GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, RENDER_WIDTH, RENDER_HEIGHT);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  // Build the framebuffer
  glGenFramebuffers(1, &self->fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
      self->color_buffer, 0);
  glFramebufferRenderbuffer(
      GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, self->depth_buffer);

  status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if (status != GL_FRAMEBUFFER_COMPLETE) {
    fprintf(stderr, "Failed to create FBO %x\n", status);
    goto err_free;
  }

  self->vertex_shader = compile_shader(vertex_shader_source, GL_VERTEX_SHADER);
  self->fragment_shader =
      compile_shader(fragment_shader_source, GL_FRAGMENT_SHADER);

  self->program = glCreateProgram();

  glAttachShader(self->program, self->vertex_shader);
  glAttachShader(self->program, self->fragment_shader);

  glBindAttribLocation(self->program, 0, "vPosition");

  glLinkProgram(self->program);

  glGetProgramiv(self->program, GL_LINK_STATUS, &linked);

  if (!linked) {
    GLint info_len = 0;

    glGetProgramiv(self->program, GL_INFO_LOG_LENGTH, &info_len);
    if (info_len > 1) {
      char* info_log = malloc(sizeof(char) * info_len);

      glGetProgramInfoLog(self->program, info_len, NULL, info_log);
      fprintf(stderr, "Error linking program:\n%s\n", info_log);
      free(info_log);
    }

    goto err_program;
  }

  wl_event_source_timer_update(self->repaint_source, REPAINT_INTERVAL);

  return self;

err_program:
  glDeleteProgram(self->program);
  glDeleteShader(self->vertex_shader);
  glDeleteShader(self->fragment_shader);

err_free:
  free(self);

err:
  return NULL;
}

void
offscreen_destroy(struct offscreen* self)
{
  glDeleteTextures(1, &self->color_buffer);
  glDeleteRenderbuffers(1, &self->depth_buffer);
  glDeleteFramebuffers(1, &self->fbo);
  wl_event_source_timer_update(self->repaint_source, 0);
  wl_event_source_remove(self->repaint_source);
  free(self);
}
