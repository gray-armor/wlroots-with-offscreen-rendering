#include <stdio.h>
#include <time.h>
#include <wlr/render/egl.h>
#include <wlr/render/gles2.h>
#include <wlr/types/wlr_output.h>

#include "internal.h"

static void output_destroy(struct output *self);

static void
output_destroy_handler(struct wl_listener *listener, void *data)
{
  struct output *output = wl_container_of(listener, output, destroy_listener);
  UNUSED(data);

  output_destroy(output);
}

static void
output_frame_handler(struct wl_listener *listener, void *data)
{
  struct output *self = wl_container_of(listener, self, frame_listener);
  struct wlr_renderer *renderer = self->wlr_output->renderer;
  struct timespec now;
  long msec;
  float red;
  UNUSED(data);

  clock_gettime(CLOCK_MONOTONIC, &now);
  msec = now.tv_nsec / 1000000 + now.tv_sec * 1000;
  red = (float)(msec % 2000 - 1000) / 1000.0;
  red = red > 0 ? red : -red;

  wlr_output_attach_render(self->wlr_output, NULL);

  wlr_renderer_begin(
      renderer, self->wlr_output->width, self->wlr_output->height);

  wlr_renderer_clear(renderer, (float[]){red, 0.3, 0.2, 1});

  wlr_renderer_end(renderer);

  wlr_output_commit(self->wlr_output);
}

struct output *
output_create(struct wlr_output *wlr_output, struct wl_display *display)
{
  struct output *self;
  struct wlr_output_mode *mode;
  struct wlr_egl *egl = wlr_gles2_renderer_get_egl(wlr_output->renderer);

  self = zalloc(sizeof *self);
  if (self == NULL) {
    fprintf(stderr, "No memory\n");
    goto err;
  }

  self->wlr_output = wlr_output;

  self->destroy_listener.notify = output_destroy_handler;
  wl_signal_add(&self->wlr_output->events.destroy, &self->destroy_listener);

  self->frame_listener.notify = output_frame_handler;
  wl_signal_add(&self->wlr_output->events.frame, &self->frame_listener);

  mode = wlr_output_preferred_mode(self->wlr_output);
  if (mode == NULL) {
    fprintf(stderr, "Failed to get preferred output mode\n");
    goto err_signals;
  }

  self->offscreen = offscreen_create(egl, display);
  if (self->offscreen == NULL) {
    fprintf(stderr, "Failed to get offscreen\n");
    goto err_signals;
  }

  wlr_output_set_mode(self->wlr_output, mode);
  wlr_output_enable(self->wlr_output, true);
  if (wlr_output_commit(self->wlr_output) == false) {
    fprintf(stderr, "Failed to set output mode\n");
    goto err_offscreen;
  }

  return self;

err_offscreen:
  offscreen_destroy(self->offscreen);

err_signals:
  wl_list_remove(&self->destroy_listener.link);
  wl_list_remove(&self->frame_listener.link);

err:
  return NULL;
}

static void
output_destroy(struct output *self)
{
  offscreen_destroy(self->offscreen);
  wl_list_remove(&self->destroy_listener.link);
  wl_list_remove(&self->frame_listener.link);
  free(self);
}
