#include "app.h"
#include "lib/glad/glad.h"
#include "gl.h"
#include "world.h"
#include "gui.h"
#include "windows.h"
#include <pthread.h>

/*-- app --*/

app app_new(int width, int height, const char *name) {
  if (!glfw_init()) {
    throw_c("Failed to initialize GLFW!");
  }

  GLFWwindow *win;

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

  buf post_vbo = buf_new(GL_ARRAY_BUFFER);

  buf_data_n(&post_vbo, GL_DYNAMIC_DRAW, sizeof(v2f), 6,
             (v2f[]){
               {0, 0},
               {1, 0},
               {1, 1},
               {1, 1},
               {0, 1},
               {0, 0},
             });

  return (app){
    .glfw_win = win,
    .dim = {(float)width, (float)height},
    .is_mouse_captured = true,
    .post = vao_new(&post_vbo, NULL, 1, (attrib[]){attr_2f}),
    .cam = cam_new((v3f){0.f, 20.f, 0.f}, (v3f){0.f, 1.f, 0.f}, 225.f, -30.f,
                   (float)width / (float)height),
    .main = fbo_new(2,
                    (fbo_spec[]){
                      {GL_COLOR_ATTACHMENT0, tex_spec_rgba8(width, height,
                                                            GL_LINEAR)},
                      {GL_DEPTH_ATTACHMENT,  tex_spec_depth24(width, height,
                                                              GL_NEAREST)}
                    }),
    .cmyk = fbo_new(1, (fbo_spec[]){
      {GL_COLOR_ATTACHMENT0, tex_spec_rgba16(width, height, GL_LINEAR)}}),
    .cmyk2 = fbo_new(1, (fbo_spec[]){
      {GL_COLOR_ATTACHMENT0, tex_spec_rgba16(width, height, GL_LINEAR)}}),
    .to_cmyk = shader_new(2,
                          (shader_spec[]){
                            {GL_VERTEX_SHADER,   "res/post.vsh"},
                            {GL_FRAGMENT_SHADER, "res/to_cmyk.fsh"}
                          }),
    .dots = shader_new(2,
                       (shader_spec[]){
                         {GL_VERTEX_SHADER,   "res/post.vsh"},
                         {GL_FRAGMENT_SHADER, "res/halftone.fsh"}
                       }),
    .blit = shader_new(2,
                       (shader_spec[]){
                         {GL_VERTEX_SHADER,   "res/post.vsh"},
                         {GL_FRAGMENT_SHADER, "res/blit.fsh"}
                       }),
    .blur = shader_new(2,
                       (shader_spec[]){
                         {GL_VERTEX_SHADER,   "res/post.vsh"},
                         {GL_FRAGMENT_SHADER, "res/blur.fsh"}
                       }),
    .world = world_new(),
    .font = font_new(read_bin_file("res/futura.otf"), 128, 40),
    .win = win_new("hello world!", NULL, (v2f){100, 100}, (v2f){100, 400})
  };
}

void app_setup_user_ptr(app *g) {
  glfw_set_window_user_pointer(g->glfw_win, g);
}

void move_cam(app *a, float dt) {
  float x = 0, y = 0, z = 0;

  if (!app_is_key_down(a, GLFW_KEY_LEFT_CONTROL)) {
    z += (float)app_is_key_down(a, GLFW_KEY_W);
    z -= (float)app_is_key_down(a, GLFW_KEY_S);
    x += (float)app_is_key_down(a, GLFW_KEY_D);
    x -= (float)app_is_key_down(a, GLFW_KEY_A);
    y += (float)app_is_key_down(a, GLFW_KEY_SPACE);
    y -= (float)app_is_key_down(a, GLFW_KEY_LEFT_SHIFT);
  }

  v3f x_comp = v3_mul(a->cam.right, x), y_comp = v3_mul(a->cam.world_up, y);

  v3f flat_front = {a->cam.front.x, 0.f, a->cam.front.z};
  if (v3_len(flat_front) > 0.0001) {
    v3_norm(&flat_front);
  }

  v3f z_comp = v3_mul(flat_front, z);

  v3f delta = v3_add(v3_add(x_comp, y_comp), z_comp);

  if (v3_len(delta) > 0.0001) {
    v3_norm(&delta);
  }

  a->cam.pos = v3_add(a->cam.pos, v3_mul(delta, dt * 20.f));
}

void app_tick(app *a) {
  const float tick_len = 50.f; // 20 tps
  static float last_frame_in_ticks = -10000.f;
  static float prev_time_ms = -100000.f;

  float time = (float)glfw_get_time() * 1000.f;
  if (prev_time_ms < 0.f) prev_time_ms = time;
  last_frame_in_ticks =
    (time - prev_time_ms) / tick_len; // # of ticks it took to update last time
  prev_time_ms = time;
  a->dt += last_frame_in_ticks; // last_frame_in_ticks number of ticks have passed, add that to delta time
  int i = (int)a->dt; // number of whole ticks that have passed, tick them

  auto t_start = (float)glfw_get_time();
  for (int j = 0; j < min(i, 10); j++) {
    world_tick(&a->world, &a->cam);
    auto b_start = (float)glfw_get_time();
    pthread_mutex_lock(&a->world.render_lock);
    arr_copy(&a->world.objs_draw, a->world.objs_tick);
    a->dt -= 1;
    pthread_mutex_unlock(&a->world.render_lock);
    a->mspb = ((float)glfw_get_time() - b_start) * 1000;
  }

  if (i) {
    a->mspt = ((float)glfw_get_time() - t_start) * 1000;
  }
}

void *tick_runner(void *ap) {
  app *a = ap;
  while (!glfw_window_should_close(a->glfw_win)) {
    app_tick(a);
  }

  pthread_exit(NULL);
  return NULL;
}

void app_run(app *a) {
  app_setup_user_ptr(a);
  gl_depth_func(GL_LESS);
  gl_clear_color(0.3f, 1.f, 1.f, 1.f);
  gl_enable(GL_BLEND);
  gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  gl_debug_message_callback(gl_error_callback, NULL);
  gl_enable(GL_DEBUG_OUTPUT);
//  gl_enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

  pthread_t thread;
  pthread_create(&thread, NULL, tick_runner, a);

  double last_frame = 0;

  while (!glfw_window_should_close(a->glfw_win)) {
    auto start = glfw_get_time();

    gl_enable(GL_DEPTH_TEST);

    // draw the scene
    fbo_bind(&a->main);
    gl_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    move_cam(a, (float)(start - last_frame));
    cam_rot(&a->cam);
    world_draw(&a->world, &a->cam, a->dt);
    imod_draw(&a->cam);

    if (a->is_rendering_halftone) {
      gl_disable(GL_BLEND);

      // convert to cmyk
      fbo_bind(&a->cmyk);
      gl_clear(GL_COLOR_BUFFER_BIT);

      shader_bind(&a->to_cmyk);
      to_cmyk_up(&a->to_cmyk,
                 (to_cmyk){
                   .tex = fbo_tex_at(&a->main, GL_COLOR_ATTACHMENT0),
                   .unit = 0
                 });

      vao_bind(&a->post);
      gl_draw_arrays(GL_TRIANGLES, 0, 6);

      // blur cmyk
      fbo_bind(&a->cmyk2);
      gl_clear(GL_COLOR_BUFFER_BIT);

      shader_bind(&a->blur);
      blur_up(&a->blur,
              (blur){
                .tex = fbo_tex_at(&a->cmyk, GL_COLOR_ATTACHMENT0),
                .unit = 0,
                .scr_size = a->dim
              });

      vao_bind(&a->post);
      gl_draw_arrays(GL_TRIANGLES, 0, 6);

      gl_enable(GL_BLEND);

      // draw dots on back-buffer
      gl_bind_framebuffer(GL_FRAMEBUFFER, 0);
      gl_disable(GL_DEPTH_TEST);
      gl_clear(GL_COLOR_BUFFER_BIT);

      shader_bind(&a->dots);
      halftone_up(&a->dots,
                  (halftone){
                    .cmyk = fbo_tex_at(&a->cmyk2, GL_COLOR_ATTACHMENT0),
                    .unit = 0,
                    .dots_per_line = 160,
                    .scr_size = a->dim
                  });

      vao_bind(&a->post);
      gl_draw_arrays(GL_TRIANGLES, 0, 6);
    } else {
      gl_bind_framebuffer(GL_FRAMEBUFFER, 0);
      gl_disable(GL_DEPTH_TEST);
      gl_clear(GL_COLOR_BUFFER_BIT);

      shader_bind(&a->blit);
      blit_up(&a->blit,
              (blit){
                .tex = fbo_tex_at(&a->main, GL_COLOR_ATTACHMENT0),
                .unit = 0
              });

      vao_bind(&a->post);
      gl_draw_arrays(GL_TRIANGLES, 0, 6);
    }

    gl_enable(GL_BLEND);

    char text_buf[128];
    sprintf_s(text_buf, 128, "xyz: %f %f %f", a->cam.pos.x, a->cam.pos.y,
              a->cam.pos.z);
    font_draw(&a->font, a, text_buf, (v2f){20, 20}, 0xffffffff, 1, 1.f);
    sprintf_s(text_buf, 128, "msp(t/b/f): %f/%f/%lf", a->mspt, a->mspb,
              (glfw_get_time() - start) * 1000);
    font_draw(&a->font, a, text_buf, (v2f){20, 20 + a->font.size}, 0xffffffff,
              1, 1.f);
    sprintf_s(text_buf, 128, "world size: %zu/%zu", a->world.chunks.count,
              a->world.chunks.cap);
    font_draw(&a->font, a, text_buf, (v2f){20, 20 + a->font.size * 2},
              0xffffffff, 1, 1.f);

    last_frame = start;
    glfw_swap_buffers(a->glfw_win);
    glfw_poll_events();
  }
}

void app_cleanup(app *g) {
  glfw_destroy_window(g->glfw_win);
}

bool app_is_key_down(app *g, int key) {
  return glfw_get_key(g->glfw_win, key) == GLFW_PRESS;
}

/*-- glfw callbacks --*/

void framebuffer_size_callback(GLFWwindow *win, int width, int height) {
  app *k = glfw_get_window_user_pointer(win);
  k->dim = (v2f){(float)width, (float)height};
  fbo_resize(&k->cmyk, width, height, 1, (uint[]){GL_COLOR_ATTACHMENT0});
  fbo_resize(&k->cmyk2, width, height, 1, (uint[]){GL_COLOR_ATTACHMENT0});
  fbo_resize(&k->main, width, height, 2,
             (uint[]){GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT});
  k->cam.aspect = (float)width / (float)height;
  gl_viewport(0, 0, width, height);
}

void cursor_pos_callback(GLFWwindow *win, double xpos, double ypos) {
  app *k = glfw_get_window_user_pointer(win);
  k->mouse = (v2f){(float)xpos, (float)ypos};
  cam_tick(&k->cam, k);
}

void
key_callback(GLFWwindow *win, int keycode, int scancode, int action, int mods) {
  app *g = glfw_get_window_user_pointer(win);
  switch (keycode) {
    case GLFW_KEY_ESCAPE: {
      if (action != GLFW_PRESS) break;
      glfw_set_input_mode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      g->is_mouse_captured = false;
      g->cam.has_last = false;
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
mouse_button_callback(GLFWwindow *win, int keycode, int action, int mods) {
  app *a = glfw_get_window_user_pointer(win);
  switch (keycode) {
    case GLFW_MOUSE_BUTTON_LEFT: {
      if (action != GLFW_PRESS) break;
      if (win_click(&a->win, a->mouse)) break;
      glfw_set_input_mode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      a->is_mouse_captured = true;
      break;
    }
  }
}

void
gl_error_callback(uint source, uint type, uint id, uint severity, int length,
                  char const *message, void const *user_param) {
  printf("%s\n", message);
}
