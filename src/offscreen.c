#include <stdio.h>
#include <wlr/render/egl.h>

#include "internal.h"

#define REPAINT_INTERVAL 10
#define RENDER_WIDTH 600
#define RENDER_HEIGHT 600

int
offscreen_repaint(void* data)
{
  struct offscreen* self = data;
  wl_event_source_timer_update(self->repaint_source, REPAINT_INTERVAL);

  return 1;
}

struct offscreen*
offscreen_create(struct wlr_egl* egl, struct wl_display* display)
{
  struct offscreen* self;
  struct wl_event_loop* loop = wl_display_get_event_loop(display);

  self = zalloc(sizeof *self);
  if (self == NULL) {
    fprintf(stderr, "No memory\n");
    goto err;
  }

  self->egl = egl;
  self->repaint_source = wl_event_loop_add_timer(loop, offscreen_repaint, self);
  wl_event_source_timer_update(self->repaint_source, REPAINT_INTERVAL);

  return self;

err:
  return NULL;
}

void
offscreen_destroy(struct offscreen* self)
{
  wl_event_source_timer_update(self->repaint_source, 0);
  wl_event_source_remove(self->repaint_source);
  free(self);
}
