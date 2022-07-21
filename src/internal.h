#ifndef WLR_OFF_INTERNAL_H
#define WLR_OFF_INTERNAL_H

#include <stdlib.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/gles2.h>
#include <wlr/types/wlr_output.h>

/** Suppress compiler warnings for unused variables */
#ifndef UNUSED
#define UNUSED(x) ((void)x)
#endif

/** Allocate memory and set to zero */
static inline void *
zalloc(size_t size)
{
  return calloc(1, size);
}

/** Compile-time computation of number of items in an array */
#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a)[0])
#endif

struct server {
  struct wl_display *display;
  struct wlr_backend *backend;
  struct wlr_renderer *renderer;  // gles2 renderer;
  struct wlr_allocator *allocator;

  struct wl_listener new_output_listener;
};

bool server_start(struct server *self);

struct server *server_create(struct wl_display *display);

void server_destroy(struct server *self);

struct offscreen {
  struct wlr_egl *egl;

  struct wl_event_source *repaint_source;
};

struct offscreen *offscreen_create(
    struct wlr_egl *egl, struct wl_display *display);

void offscreen_destroy(struct offscreen *self);

struct output {
  struct wlr_output *wlr_output;

  struct offscreen *offscreen;

  struct wl_listener destroy_listener;
  struct wl_listener frame_listener;
};

struct output *output_create(
    struct wlr_output *output, struct wl_display *display);

#endif  //  WLR_OFF_INTERNAL_H
