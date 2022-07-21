#include <stdio.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_output.h>

#include "internal.h"

static void
server_new_output_handler(struct wl_listener *listener, void *data)
{
  struct wlr_output *wlr_output = data;
  struct server *self = wl_container_of(listener, self, new_output_listener);
  struct output *output;

  if (wlr_output->non_desktop) {
    fprintf(stderr, "Non configuring non-desktop output\n");
    return;
  }

  if (wlr_output_init_render(wlr_output, self->allocator, self->renderer) ==
      false) {
    fprintf(stderr, "Failed to initialize output renderer\n");
    return;
  }

  output = output_create(wlr_output, self->display);
  if (output == NULL) {
    fprintf(stderr, "Failed to create a output\n");
    return;
  }
}

bool
server_start(struct server *self)
{
  return wlr_backend_start(self->backend);
}

struct server *
server_create(struct wl_display *display)
{
  struct server *self;
  int drm_fd = -1;

  self = zalloc(sizeof *self);
  if (self == NULL) {
    fprintf(stderr, "No memory\n");
    goto err;
  }

  self->display = display;

  self->backend = wlr_backend_autocreate(self->display);
  if (self->backend == NULL) {
    fprintf(stderr, "Failed to create a backend\n");
    goto err_free;
  }

  drm_fd = wlr_backend_get_drm_fd(self->backend);
  if (drm_fd < 0) {
    fprintf(stderr, "Failed to get drm fd\n");
    goto err_backend;
  }

  self->renderer = wlr_gles2_renderer_create_with_drm_fd(drm_fd);
  if (self->renderer == NULL) {
    fprintf(stderr, "Failed to create renderer\n");
    goto err_backend;
  }

  if (wlr_renderer_init_wl_display(self->renderer, self->display) == false) {
    fprintf(stderr, "Failed to initialize renderer with wl_display\n");
    goto err_renderer;
  }

  self->allocator = wlr_allocator_autocreate(self->backend, self->renderer);
  if (self->allocator == NULL) {
    fprintf(stderr, "Failed to create allocator\n");
    goto err_renderer;
  }

  self->new_output_listener.notify = server_new_output_handler;
  wl_signal_add(&self->backend->events.new_output, &self->new_output_listener);

  return self;

err_renderer:
  wlr_renderer_destroy(self->renderer);

err_backend:
  wlr_backend_destroy(self->backend);

err_free:
  free(self);

err:
  return NULL;
}

void
server_destroy(struct server *self)
{
  wlr_allocator_destroy(self->allocator);
  wlr_renderer_destroy(self->renderer);
  wlr_backend_destroy(self->backend);
  free(self);
}
