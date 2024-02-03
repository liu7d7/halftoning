#pragma once

#include "typedefs.h"

#include "lib/glfw/include/GLFW/glfw3.h"
#include "err.h"
#include "gl.h"

typedef struct game_t {
  vec2 win_size;
  vec2 mouse_pos;
  vao_t vao;
  shader_t shader;
  cam_t cam;
  bool is_mouse_captured;

  // owning!
  GLFWwindow* win;
} game_t;

game_t game(int width, int height, char const* name);

void game_run(game_t* g);
void game_cleanup(game_t* g);
void game_setup_user_ptr(game_t* g);
bool game_is_key_down(game_t* g, int key);

void framebuffer_size_callback(GLFWwindow* win, int width, int height);
void cursor_pos_callback(GLFWwindow* win, double xpos, double ypos);
void key_callback(GLFWwindow* win, int keycode, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* win, int button, int action, int mods);
void gl_error_callback(uint source, uint type, uint id, uint severity, int length, char const* message, void const* user_param);