#include "game.h"
#include "lib/glad/glad.h"
#include "gl.h"

/*-- game --*/

game_t game(int width, int height, const char* name) {
  if (!glfw_init()) {
    throw_c("Failed to initialize GLFW!");
  }

  game_t g = {
    .win_size = {(float)width, (float)height},
    .is_mouse_captured = true
  };

  glfw_window_hint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfw_window_hint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfw_window_hint(GLFW_VERSION_MAJOR, 3);
  glfw_window_hint(GLFW_VERSION_MINOR, 3);
  glfw_window_hint(GLFW_RESIZABLE, GLFW_TRUE);
  glfw_window_hint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  glfw_window_hint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfw_window_hint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
  if ((g.win = glfw_create_window(width, height, name, nullptr, nullptr)) ==
      nullptr) {
    throw_c("Failed to create a GLFW window!");
  }

  glfw_make_context_current(g.win);
  glfw_set_framebuffer_size_callback(g.win, framebuffer_size_callback);
  glfw_set_cursor_pos_callback(g.win, cursor_pos_callback);
  glfw_set_key_callback(g.win, key_callback);
  glfw_set_mouse_button_callback(g.win, mouse_button_callback);
  glfw_swap_interval(1);

  if (!glad_load_gl_loader((GLADloadproc)glfw_get_proc_address)) {
    throw_c("Failed to load GLAD!");
  }

  glfw_set_input_mode(g.win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  buf_t* vbo = buf_heap_new(GL_ARRAY_BUFFER);
  buf_t* ibo = buf_heap_new(GL_ELEMENT_ARRAY_BUFFER);

  buf_data_n(vbo, GL_DYNAMIC_STORAGE_BIT, sizeof(vec3), 8,
             (vec3[]){
               {-20.f, -20.f, 0.f},
               {-20.f, 20.f,  0.f},
               {20.f,  20.f,  0.f},
               {20.f,  -20.f, 0.f},
               {-20.f, -20.f, 30.f},
               {-20.f, 20.f,  30.f},
               {20.f,  20.f,  30.f},
               {20.f,  -20.f, 30.f},
             });

  buf_data_n(ibo, GL_DYNAMIC_STORAGE_BIT, sizeof(uint), 12,
             (uint[]){0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4});

  g.vao = vao(vbo, ibo, 1, (attrib_t[]){attr_float_3});

  g.shader = shader(2,
                    (shader_conf_t[]){
                      {"res/vert.vsh", GL_VERTEX_SHADER},
                      {"res/frag.fsh", GL_FRAGMENT_SHADER}
                    });

  g.cam = cam((vec3){0.f, 0.f, 0.f}, (vec3){0.f, 1.f, 0.f}, 0.f, 0.f);

  return g;
}

void game_setup_user_ptr(game_t* g) {
  glfw_set_window_user_pointer(g->win, g);
}

void update_camera(game_t* g) {
  float x = 0, y = 0, z = 0;
  z += (float)game_is_key_down(g, GLFW_KEY_W);
  z -= (float)game_is_key_down(g, GLFW_KEY_S);
  x += (float)game_is_key_down(g, GLFW_KEY_D);
  x -= (float)game_is_key_down(g, GLFW_KEY_A);
  y += (float)game_is_key_down(g, GLFW_KEY_SPACE);
  y -= (float)game_is_key_down(g, GLFW_KEY_LEFT_SHIFT);

  vec3 x_comp, y_comp, z_comp;
  glm_vec3_zero(x_comp);
  glm_vec3_zero(y_comp);
  glm_vec3_zero(z_comp);
  glm_vec3_mul(g->cam.right, (vec3){x, x, x}, x_comp);
  glm_vec3_mul(g->cam.world_up, (vec3){y, y, y}, y_comp);

  vec3 flat_front = {g->cam.front[0], 0, g->cam.front[2]};
  if (glm_vec3_norm(flat_front) > 0.0001) {
    glm_vec3_normalize(flat_front);
  }

  glm_vec3_mul(flat_front, (vec3){z, z, z}, z_comp);

  vec3 delta;
  glm_vec3_zero(delta);
  glm_vec3_add(delta, x_comp, delta);
  glm_vec3_add(delta, y_comp, delta);
  glm_vec3_add(delta, z_comp, delta);

  if (glm_vec3_norm(delta) > 0.0001) {
    glm_vec3_normalize(delta);
  }

  glm_vec3_add(g->cam.pos, delta, g->cam.pos);

  cam_update(&g->cam, g);
}

void game_run(game_t* g) {
  game_setup_user_ptr(g);
  gl_enable(GL_DEPTH_TEST);
  gl_depth_func(GL_LEQUAL);

  gl_debug_message_callback(gl_error_callback, NULL);
  gl_enable(GL_DEBUG_OUTPUT);
  gl_enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

  while (!glfw_window_should_close(g->win)) {
    gl_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // update the camera
    update_camera(g);

    // set up the shader
    mat4 m;
    glm_mat4_identity(m);

    cam_proj(&g->cam, g->win_size, m);
    shader_mat4(&g->shader, "u_proj", m);
    glm_mat4_zero(m);
    cam_look(&g->cam, m);
    shader_mat4(&g->shader, "u_look", m);
    shader_bind(&g->shader);

    // draw the thing
    vao_bind(&g->vao);
    gl_draw_elements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);

    glfw_swap_buffers(g->win);
    glfw_poll_events();
  }
}

void game_cleanup(game_t* g) {
  glfw_destroy_window(g->win);
}

bool game_is_key_down(game_t* g, int key) {
  return glfw_get_key(g->win, key) == GLFW_PRESS;
}

/*-- glfw callbacks --*/

void framebuffer_size_callback(GLFWwindow* win, int width, int height) {
  game_t* k = glfw_get_window_user_pointer(win);
  vec2cpy((vec2){(float)width, (float)height}, k->win_size);
  gl_viewport(0, 0, width, height);
}

void cursor_pos_callback(GLFWwindow* win, double xpos, double ypos) {
  game_t* k = glfw_get_window_user_pointer(win);
  vec2cpy((vec2){(float)xpos, (float)ypos}, k->mouse_pos);
}

void
key_callback(GLFWwindow* win, int keycode, int scancode, int action, int mods) {
  game_t* g = glfw_get_window_user_pointer(win);
  switch (keycode) {
    case GLFW_KEY_ESCAPE: {
      if (action != GLFW_PRESS) break;
      glfw_set_input_mode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      g->is_mouse_captured = false;
      break;
    }
  }
}

void
mouse_button_callback(GLFWwindow* win, int keycode, int action, int mods) {
  game_t* g = glfw_get_window_user_pointer(win);
  switch (keycode) {
    case GLFW_MOUSE_BUTTON_LEFT: {
      if (action != GLFW_PRESS) break;
      glfw_set_input_mode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      g->is_mouse_captured = true;
      break;
    }
  }
}

void
gl_error_callback(uint source, uint type, uint id, uint severity, int length,
                  char const* message, void const* user_param) {
  printf("%s\n", message);
}
