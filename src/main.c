#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wlr/util/log.h>

#include "internal.h"

struct server *server;

static int
on_term_signal(int signal, void *data)
{
  struct wl_display *display = data;

  fprintf(stderr, "Caught signal: %d\n", signal);
  wl_display_terminate(display);

  return 0;
}

int
main()
{
  struct wl_display *display;
  struct wl_event_source *signal_sources[3];
  struct wl_event_loop *loop;
  int i, ret = EXIT_FAILURE;

  wlr_log_init(WLR_DEBUG, NULL);

  display = wl_display_create();

  server = server_create(display);
  if (server == NULL) {
    fprintf(stderr, "Hello World\n");
    goto err;
  }

  loop = wl_display_get_event_loop(display);
  signal_sources[0] =
      wl_event_loop_add_signal(loop, SIGTERM, on_term_signal, display);
  signal_sources[1] =
      wl_event_loop_add_signal(loop, SIGINT, on_term_signal, display);
  signal_sources[2] =
      wl_event_loop_add_signal(loop, SIGQUIT, on_term_signal, display);

  if (!signal_sources[0] || !signal_sources[1] || !signal_sources[2]) {
    fprintf(stderr, "Failed to add signal handler\n");
    goto err_server;
  }

  if (!server_start(server)) goto err_signals;

  wl_display_run(display);

  ret = EXIT_SUCCESS;

err_signals:
  for (i = ARRAY_LENGTH(signal_sources) - 1; i >= 0; i--)
    if (signal_sources[i]) wl_event_source_remove(signal_sources[i]);

err_server:
  server_destroy(server);

err:
  return ret;
}
