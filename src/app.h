#pragma once

#include <pthread.h>
#include "typedefs.h"

#include "lib/glfw/include/GLFW/glfw3.h"
#include "err.h"
#include "gl.h"
#include "world.h"
#include "text.h"
#include "gui.h"
#include "avg.h"
#include "arena.h"
#include "ani.h"

typedef struct app {
  v2 dim;
  iv2 lo_dim;
  v2 mouse;
  vao post;
  shdr dither, blit, crt, outline;
  cam cam, shade_cam;
  fbo low_res, low_res_2, main, shade;
  world *world;
  bool is_mouse_captured, is_rendering_halftone;
  float dt;
  avg_num mspt, mspf, mspd;
  text text;
  win win;
  int player;
  arena temp;
  mod ball;
  ani_mod cyl;

  // debug info
  int n_drawn, n_close;
  size_t n_tris;

  // owning!
  GLFWwindow *glfw_win;
} app;

extern app $;

app app_new(int width, int height, char const *name);

void app_run(app *a);

void app_cleanup(app *g);

void app_setup_user_ptr(app *g);

bool app_is_key_down(app *g, int key);

void framebuffer_size_callback(GLFWwindow *win, int width, int height);

void cursor_pos_callback(GLFWwindow *win, double xpos, double ypos);

void
key_callback(GLFWwindow *win, int keycode, int scancode, int action, int mods);

void mouse_button_callback(GLFWwindow *win, int button, int action, int mods);

void
gl_error_callback(u32 source, u32 type, u32 id, u32 severity, int length,
                  char const *message, void const *user_param);

float app_now();