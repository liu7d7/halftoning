#include "game.h"
#include "lib/glad/glad.h"
#include "gl.h"
#include "world.h"

/*-- game --*/

struct game game(int width, int height, const char* name) {
  if (!glfw_init()) {
    throw_c("Failed to initialize GLFW!");
  }

  GLFWwindow* win;

  glfw_window_hint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfw_window_hint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfw_window_hint(GLFW_VERSION_MAJOR, 3);
  glfw_window_hint(GLFW_VERSION_MINOR, 3);
  glfw_window_hint(GLFW_RESIZABLE, GLFW_TRUE);
  glfw_window_hint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  glfw_window_hint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfw_window_hint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
  if ((win = glfw_create_window(width, height, name, NULL, NULL)) == NULL) {
    throw_c("Failed to create a GLFW window!");
  }

  glfw_make_context_current(win);
  glfw_set_framebuffer_size_callback(win, framebuffer_size_callback);
  glfw_set_cursor_pos_callback(win, cursor_pos_callback);
  glfw_set_key_callback(win, key_callback);
  glfw_set_mouse_button_callback(win, mouse_button_callback);
  glfw_swap_interval(1);

  if (!glad_load_gl_loader((GLADloadproc)glfw_get_proc_address)) {
    throw_c("Failed to load GLAD!");
  }

  glfw_set_input_mode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  struct buf post_vbo = buf(GL_ARRAY_BUFFER);

  buf_data_n(&post_vbo, GL_DYNAMIC_STORAGE_BIT, sizeof(vec2), 6,
             (vec2[]){
               {0, 0},
               {1, 0},
               {1, 1},
               {1, 1},
               {0, 1},
               {0, 0},
             });

  return (struct game){
    .win = win,
    .win_size = {(float)width, (float)height},
    .is_mouse_captured = true,
    .post = vao(&post_vbo, NULL, 1, (struct attrib[]){attr_2f}),
    .cam = cam((vec3){0.f, 50.f, 0.f}, (vec3){0.f, 1.f, 0.f}, 225.f, -30.f, (float)width / (float)height),
    .main = fbo(2,
                (struct fbo_spec[]){
                  {GL_COLOR_ATTACHMENT0, tex_spec_rgba8(width, height,
                                                        GL_LINEAR)},
                  {GL_DEPTH_ATTACHMENT,  tex_spec_depth24(width, height,
                                                          GL_NEAREST)}
                }),
    .cmyk = fbo(1, (struct fbo_spec[]){
      {GL_COLOR_ATTACHMENT0, tex_spec_rgba16(width, height, GL_LINEAR)}}),
    .cmyk2 = fbo(1, (struct fbo_spec[]){
      {GL_COLOR_ATTACHMENT0, tex_spec_rgba16(width, height, GL_LINEAR)}}),
    .to_cmyk = shader(2,
                      (struct shader_spec[]){
                        {GL_VERTEX_SHADER,   "res/post.vsh"},
                        {GL_FRAGMENT_SHADER, "res/to_cmyk.fsh"}
                      }),
    .dots = shader(2,
                   (struct shader_spec[]){
                     {GL_VERTEX_SHADER,   "res/post.vsh"},
                     {GL_FRAGMENT_SHADER, "res/halftone.fsh"}
                   }),
    .blit = shader(2,
                   (struct shader_spec[]){
                     {GL_VERTEX_SHADER,   "res/post.vsh"},
                     {GL_FRAGMENT_SHADER, "res/blit.fsh"}
                   }),
    .blur = shader(2,
                   (struct shader_spec[]){
                     {GL_VERTEX_SHADER,   "res/post.vsh"},
                     {GL_FRAGMENT_SHADER, "res/blur.fsh"}
                   }),
    .world = world()
  };
}

void game_setup_user_ptr(struct game* g) {
  glfw_set_window_user_pointer(g->win, g);
}

void update_camera(struct game* g) {
  float x = 0, y = 0, z = 0;

  if (!game_is_key_down(g, GLFW_KEY_LEFT_CONTROL)) {
    z += (float)game_is_key_down(g, GLFW_KEY_W);
    z -= (float)game_is_key_down(g, GLFW_KEY_S);
    x += (float)game_is_key_down(g, GLFW_KEY_D);
    x -= (float)game_is_key_down(g, GLFW_KEY_A);
    y += (float)game_is_key_down(g, GLFW_KEY_SPACE);
    y -= (float)game_is_key_down(g, GLFW_KEY_LEFT_SHIFT);
  }

  vec3 x_comp, y_comp, z_comp;
  glm_vec3_zero(x_comp);
  glm_vec3_zero(y_comp);
  glm_vec3_zero(z_comp);
  glm_vec3_mul(g->cam.right, (vec3){x, x, x}, x_comp);
  glm_vec3_mul(g->cam.world_up, (vec3){y, y, y}, y_comp);

  vec3 flat_front = {g->cam.front[0], 0.f, g->cam.front[2]};
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

void game_run(struct game* g) {
  game_setup_user_ptr(g);
  gl_depth_func(GL_LESS);
  gl_clear_color(0.3f, 1.f, 1.f, 1.f);

  gl_debug_message_callback(gl_error_callback, NULL);
  gl_enable(GL_DEBUG_OUTPUT);
  gl_enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

  while (!glfw_window_should_close(g->win)) {
    gl_enable(GL_DEPTH_TEST);

    // draw the scene
    fbo_bind(&g->main);
    gl_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // update the camera
    update_camera(g);

    // draw the thing
    world_draw(&g->world, &g->cam);

    if (g->is_rendering_halftone) {
      // convert to cmyk
      fbo_bind(&g->cmyk);
      gl_clear(GL_COLOR_BUFFER_BIT);

      shader_bind(&g->to_cmyk);
      to_cmyk_up(&g->to_cmyk,
                 (struct to_cmyk){
                   .tex = fbo_tex_at(&g->main, GL_COLOR_ATTACHMENT0),
                   .unit = 0
                 });

      vao_bind(&g->post);
      gl_draw_arrays(GL_TRIANGLES, 0, 6);

      // blur cmyk
      fbo_bind(&g->cmyk2);
      gl_clear(GL_COLOR_BUFFER_BIT);

      shader_bind(&g->blur);
      blur_up(&g->blur,
              (struct blur){
                .tex = fbo_tex_at(&g->cmyk, GL_COLOR_ATTACHMENT0),
                .unit = 0,
                .scr_size = VEC2_COPY_INIT(g->win_size)
              });

      vao_bind(&g->post);
      gl_draw_arrays(GL_TRIANGLES, 0, 6);

      // draw dots on back-buffer
      gl_bind_framebuffer(GL_FRAMEBUFFER, 0);
      gl_disable(GL_DEPTH_TEST);
      gl_clear(GL_COLOR_BUFFER_BIT);

      shader_bind(&g->dots);
      halftone_up(&g->dots,
                  (struct halftone){
                    .cmyk = fbo_tex_at(&g->cmyk2, GL_COLOR_ATTACHMENT0),
                    .unit = 0,
                    .dots_per_line = 160,
                    .scr_size = VEC2_COPY_INIT(g->win_size)
                  });

      vao_bind(&g->post);
      gl_draw_arrays(GL_TRIANGLES, 0, 6);
    } else {
      gl_bind_framebuffer(GL_FRAMEBUFFER, 0);
      gl_disable(GL_DEPTH_TEST);
      gl_clear(GL_COLOR_BUFFER_BIT);

      shader_bind(&g->blit);
      blit_up(&g->blit,
              (struct blit){
                .tex = fbo_tex_at(&g->main, GL_COLOR_ATTACHMENT0),
                .unit = 0
              });

      vao_bind(&g->post);
      gl_draw_arrays(GL_TRIANGLES, 0, 6);
    }

    glfw_swap_buffers(g->win);
    glfw_poll_events();
  }
}

void game_cleanup(struct game* g) {
  glfw_destroy_window(g->win);
}

bool game_is_key_down(struct game* g, int key) {
  return glfw_get_key(g->win, key) == GLFW_PRESS;
}

/*-- glfw callbacks --*/

void framebuffer_size_callback(GLFWwindow* win, int width, int height) {
  struct game* k = glfw_get_window_user_pointer(win);
  vec2cpy(k->win_size, (vec2){(float)width, (float)height});
  fbo_resize(&k->cmyk, width, height, 1, (uint[]){GL_COLOR_ATTACHMENT0});
  fbo_resize(&k->cmyk2, width, height, 1, (uint[]){GL_COLOR_ATTACHMENT0});
  fbo_resize(&k->main, width, height, 2,
             (uint[]){GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT});
  k->cam.aspect = (float)width / (float)height;
  gl_viewport(0, 0, width, height);
}

void cursor_pos_callback(GLFWwindow* win, double xpos, double ypos) {
  struct game* k = glfw_get_window_user_pointer(win);
  vec2cpy(k->mouse_pos, (vec2){(float)xpos, (float)ypos});
}

void
key_callback(GLFWwindow* win, int keycode, int scancode, int action, int mods) {
  struct game* g = glfw_get_window_user_pointer(win);
  switch (keycode) {
    case GLFW_KEY_ESCAPE: {
      if (action != GLFW_PRESS) break;
      glfw_set_input_mode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      g->is_mouse_captured = false;
      break;
    }
    case GLFW_KEY_H: {
      if (action != GLFW_PRESS) break;
      g->is_rendering_halftone = !g->is_rendering_halftone;
      break;
    }
  }
}

void
mouse_button_callback(GLFWwindow* win, int keycode, int action, int mods) {
  struct game* g = glfw_get_window_user_pointer(win);
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
