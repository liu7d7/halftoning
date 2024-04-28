#pragma once

#include <pthread.h>
#include "typedefs.h"

#include "lib/glfw/include/GLFW/glfw3.h"
#include "err.h"
#include "gl.h"
#include "world.h"
#include "font.h"
#include "gui.h"

typedef struct app {
  v2f dim;
  v2f mouse;
  vao post;
  shader dither, blit, crt;
  cam cam;
  fbo low_res, low_res_2, main;
  world *world;
  bool is_mouse_captured, is_rendering_halftone;
  float dt, mspt;
  font font;
  win win;
  int player;

  // owning!
  GLFWwindow *glfw_win;
} app;

extern app the_app;

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
gl_error_callback(uint source, uint type, uint id, uint severity, int length,
                  char const *message, void const *user_param);