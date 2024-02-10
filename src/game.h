#pragma once

#include "typedefs.h"

#include "lib/glfw/include/GLFW/glfw3.h"
#include "err.h"
#include "gl.h"

struct game {
  vec2 win_size;
  vec2 mouse_pos;
  struct vao post;
  struct shader to_cmyk, dots, blur, blit;
  struct cam cam;
  struct fbo cmyk, cmyk2, main;
  struct mod cube;
  bool is_mouse_captured, is_rendering_halftone;

  // owning!
  GLFWwindow* win;
};

struct game game(int width, int height, char const* name);

void game_run(struct game* g);
void game_cleanup(struct game* g);
void game_setup_user_ptr(struct game* g);
bool game_is_key_down(struct game* g, int key);

void framebuffer_size_callback(GLFWwindow* win, int width, int height);
void cursor_pos_callback(GLFWwindow* win, double xpos, double ypos);
void key_callback(GLFWwindow* win, int keycode, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* win, int button, int action, int mods);
void gl_error_callback(uint source, uint type, uint id, uint severity, int length, char const* message, void const* user_param);