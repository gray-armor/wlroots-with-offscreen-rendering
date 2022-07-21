_soruces = [
  'main.c',
  'offscreen.c',
  'output.c',
  'server.c',
]

_dependencies = [
  glesv2_dep,
  wayland_server_dep,
  wlroots_dep,
]

executable(
  'wlr-off',
  _soruces,
  install: false,
  dependencies: _dependencies,
)
