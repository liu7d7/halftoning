#include "app.h"
#include "lib/glad/glad.h"
#include "gl.h"
#include "world.h"
#include "gui.h"
#include <time.h>
#include "pal.h"
#include <pthread.h>

/*-- app --*/

float const low_res = 480.f;
float const shade_aspect = 1.5f;
iv2 const shade_dim = (iv2){(int)(8192 * shade_aspect), 8192};
app $;

app app_new(int width, int height, const char *name) {
  if (!glfw_init()) {
    throwf("Failed to initialize GLFW!");
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
    throwf("Failed to create a GLFW window!");
  }

  glfw_make_context_current(win);
  glfw_set_framebuffer_size_callback(win, framebuffer_size_callback);
  glfw_set_cursor_pos_callback(win, cursor_pos_callback);
  glfw_set_key_callback(win, key_callback);
  glfw_set_mouse_button_callback(win, mouse_button_callback);
  glfw_swap_interval(0);

  if (!glad_load_gl_loader((GLADloadproc)glfw_get_proc_address)) {
    throwf("Failed to load GLAD!");
  }

  glfw_set_input_mode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  buf post_vbo = buf_new(GL_ARRAY_BUFFER);

  buf_data_n(&post_vbo, GL_DYNAMIC_DRAW, sizeof(v2), 6,
             (v2[]){
               {0, 0},
               {1, 0},
               {1, 1},
               {1, 1},
               {0, 1},
               {0, 0},
             });

  iv2 lo_dim = (iv2){(int)(low_res * (float)width / (float)height),
                     (int)low_res};

  return (app){
    .glfw_win = win,
    .dim = {(float)width, (float)height},
    .lo_dim = lo_dim,
    .is_mouse_captured = true,
    .post = vao_new(&post_vbo, NULL, 1, (attrib[]){attr_2f}),
    .cam = cam_new((v3){0.f, 20.f, 0.f}, (v3){0.f, 1.f, 0.f}, 225.f, -30.f,
                   (float)width / (float)height),
    .shade_cam = cam_new((v3){0.f, 20.f, 0.f}, (v3){0.f, 1.f, 0.f}, 45.f,
                         -54.7356103172f, shade_aspect),
    .main = fbo_new(3,
                    (fbo_spec[]){
                      {GL_COLOR_ATTACHMENT0, tex_spec_rgba8(lo_dim.x * 2,
                                                            lo_dim.y * 2,
                                                            GL_LINEAR)},
                      {GL_COLOR_ATTACHMENT1, tex_spec_r32i(lo_dim.x * 2,
                                                           lo_dim.y * 2,
                                                           GL_NEAREST)},
                      {GL_DEPTH_ATTACHMENT,  tex_spec_depth32(lo_dim.x * 2,
                                                              lo_dim.y * 2,
                                                              GL_NEAREST)}
                    }),
    .shade = fbo_new(1,
                     (fbo_spec[]){
                       {GL_DEPTH_ATTACHMENT,
                        tex_spec_depth32(shade_dim.x, shade_dim.y, GL_LINEAR)}
                     }),
    .low_res = fbo_new(1, (fbo_spec[]){
      {GL_COLOR_ATTACHMENT0,
       tex_spec_rgba16(lo_dim.x, lo_dim.y, GL_NEAREST)}}),
    .low_res_2 = fbo_new(1, (fbo_spec[]){
      {GL_COLOR_ATTACHMENT0,
       tex_spec_rgba16(lo_dim.x, lo_dim.y, GL_LINEAR)}}),
    .dither = shdr_new(2,
                       (shdr_s[]){
                         {GL_VERTEX_SHADER,   "res/post.vsh"},
                         {GL_FRAGMENT_SHADER, "res/dither.fsh"}
                       }),
    .blit = shdr_new(2,
                     (shdr_s[]){
                       {GL_VERTEX_SHADER,   "res/post.vsh"},
                       {GL_FRAGMENT_SHADER, "res/blit.fsh"}
                     }),
    .crt = shdr_new(2,
                    (shdr_s[]){
                      {GL_VERTEX_SHADER,   "res/post.vsh"},
                      {GL_FRAGMENT_SHADER, "res/crt.fsh"}
                    }),
    .outline = shdr_new(2,
                        (shdr_s[]){
                          {GL_VERTEX_SHADER,   "res/post.vsh"},
                          {GL_FRAGMENT_SHADER, "res/outline.fsh"},
                        }),
    .mspf = avg_num_new(120), .mspt = avg_num_new(
      120), .mspd = avg_num_new(
      120),
    .world = world_new(hana_new()),
    .player = 0,
    .text = font_new((u8 *[fw_n]){
      [fw_reg] = read_bin_file("res/futura/futura-reg.ttf"),
      [fw_ita] = read_bin_file("res/futura/futura-ita.ttf"),
      [fw_bold] = read_bin_file("res/futura/futura-bold.ttf"),
      [fw_bold_ita] = read_bin_file("res/futura/futura-bold-ita.ttf"),
    }, 128, 44),
    .win = win_new("hello &bworld&r!", (v2){100, 100}, (v2){100, 400}),
    .is_rendering_halftone = 1,
    .temp = arena_new(1 << 22) /* 4 meg */,
    .ball = mod_new_indirect_mtl("res/ball.glb", "res/ball_trans.glb"),
    .cyl = ani_mod_new("res/cyl.dae")
  };
}

void app_setup_user_ptr(app *g) {
  glfw_set_window_user_pointer(g->glfw_win, g);
}

struct timespec timespec_diff(const struct timespec time1,
                              const struct timespec time0) {
  struct timespec diff = {
    .tv_sec = time1.tv_sec - time0.tv_sec,
    .tv_nsec = time1.tv_nsec - time0.tv_nsec
  };

  if (diff.tv_nsec < 0) {
    diff.tv_nsec += 1000000000;
    diff.tv_sec--;
  }

  return diff;
}

float app_now() {
  static struct timespec start;
  static int first_run = 1;
  if (first_run) {
    clock_gettime(CLOCK_REALTIME, &start);
    first_run = 0;
  }

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);

  struct timespec diff = timespec_diff(ts, start);
  auto res = (float)diff.tv_sec * 1e3f + (float)diff.tv_nsec / 1e6f;

  return res;
}

void app_tick(app *a) {
  const float tick_len = 16.f; // 60 tps
  static float last_frame_in_ticks = -10000.f;
  static float prev_time_ms = -100000.f;

  float time = app_now();
  if (prev_time_ms < 0.f) prev_time_ms = time;
  last_frame_in_ticks =
    (time - prev_time_ms) / tick_len; // # of ticks it took to update last time
  prev_time_ms = time;
  a->dt += last_frame_in_ticks; // last_frame_in_ticks number of ticks have passed, add that to delta time
  int i = (int)a->dt; // number of whole ticks that have passed, tick them

  auto t_start = app_now();
  for (int j = 0; j < min(i, 10); j++) {
    world_tick(a->world, &a->cam);
    arr_add_bulk(&a->world->objs_tick, a->world->objs_to_add);
    arr_clear(a->world->objs_to_add);

    pthread_mutex_lock(&a->world->draw_lock);
    arr_copy(&a->world->objs, a->world->objs_tick);
    world_cache(a->world,
                world_get_chunk_pos(a->world->objs_tick[a->player].body.pos));
    a->dt -= 1;
    pthread_mutex_unlock(&a->world->draw_lock);
  }

  if (i) {
    avg_num_add(&a->mspt, (app_now() - t_start));
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

void draw_graph(app *a, avg_num *v, v4 color, int bg) {
  v2 *p = arr_new_sized(v2, v->size);
  const float h = 200;
  const float w = 600;
  const float pad = 20;
  v2 c = {pad + w, a->dim.y - pad};
  if (bg) {
    draw_rect((v2){pad / 2., a->dim.y - pad - h},
              (v2){w + pad, a->dim.y - pad / 2.}, (v4){0.f, 0.f, 0.f, 0.5f});
  }

  for (int i = 1; i < v->size; i++) {
    float val = v->buf[idx_wrap(v->size, v->head - i)];
    arr_add(&p, &(v2){c.x - (w) / (float)v->size * i,
                      c.y - val / v->max * (h - 2 * pad)});
  }

  draw_line_graph(p, color);

  arr_del(p);
}

void app_run(app *a) {
  app_setup_user_ptr(a);
  gl_depth_func(GL_LESS);
  gl_clear_color(0.3f, 1.f, 1.f, 1.f);
  gl_enable(GL_BLEND);
  gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  gl_front_face(GL_CCW);

  gl_debug_message_callback(gl_error_callback, NULL);
  gl_enable(GL_DEBUG_OUTPUT);
  gl_enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

  a->shade_cam.ortho_size = 225;
  a->shade_cam.dist = 15.f;
  a->shade_cam.shade = 1;
  cam_rot(&a->shade_cam);

  pthread_t thread;
  pthread_create(&thread, NULL, tick_runner, a);

  fbo_draw_bufs(&a->main, 2,
                (u32[]){GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});

  anime ani = anime_new(animation_new("res/cyl.dae", &a->cyl));

  float frame_time = app_now();
  while (!glfw_window_should_close(a->glfw_win)) {
    auto start = app_now();
    a->n_tris = 0;

    float rdt = app_now() - frame_time;
    frame_time = app_now();

    gl_enable(GL_DEPTH_TEST);

    pthread_mutex_lock(&a->world->draw_lock);
    float dt = a->dt;
    anime_tick(&ani, rdt / 1000.f);
    gl_viewport(0, 0, shade_dim.x, shade_dim.y);
    fbo_bind(&a->shade);
    gl_clear(GL_DEPTH_BUFFER_BIT);
    a->shade_cam.pos = v3_add(obj_get_ipos(&a->world->objs[a->player], dt),
                              (v3){0, 0.75f, 0});
    cam_rot(&a->shade_cam);
    gl_front_face(GL_CW);
    world_draw(a->world, ds_shade, &a->shade_cam, dt);
    imod_draw(ds_shade, &a->shade_cam);
    ani_mod_draw(&a->cyl, &ani, ds_shade, &a->shade_cam, m4_ident, 0);
    gl_front_face(GL_CCW);

    {
      shdr *sh[] = {mod_get_sh(ds_cam, &a->cam, (mtl){}, m4_ident),
                    imod_get_sh(ds_cam, &a->cam, (mtl){}),
                    ch_get_sh(ds_cam, &a->cam),
                    ani_mod_get_sh(ds_cam, &a->cam, &ani_dummy, (mtl){}, m4_ident)};

      for (int i = 0; i < sizeof(sh) / sizeof(sh[0]); i++) {
        shdr_m4f(sh[i], "u_light_vp", a->shade_cam.vp);
        tex_bind(fbo_tex_at(&a->shade, GL_DEPTH_ATTACHMENT), 3);
        shdr_1i(sh[i], "u_light_tex", 3);
        shdr_2f(sh[i], "u_light_tex_size",
                (v2){1.f / (float)shade_dim.x, 1.f / (float)shade_dim.y});
      }
    }

    gl_viewport(0, 0, a->lo_dim.x * 2, a->lo_dim.y * 2);
    fbo_bind(&a->main);
    gl_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    a->cam.pos = v3_add(obj_get_ipos(&a->world->objs[a->player], dt),
                        (v3){0, 0.75f, 0});
    cam_rot(&a->cam);
    world_draw(a->world, ds_cam, &a->cam, dt);
    imod_draw(ds_cam, &a->cam);
    ani_mod_draw(&a->cyl, &ani, ds_cam, &a->cam, m4_ident, 0);
    pthread_mutex_unlock(&a->world->draw_lock);

    // raycast to get id in player's reach

    gl_enable(GL_BLEND);

//    shdr_bind(&a->outline);
//    shdr_1i(&a->outline, "u_id", id);
//    tex_bind(fbo_tex_at(&a->main, GL_COLOR_ATTACHMENT1), 0);
//    shdr_1i(&a->outline, "u_tex", 0);
//
//    vao_bind(&a->post);
//    gl_draw_arrays(GL_TRIANGLES, 0, 6);

    gl_blit_named_framebuffer(a->main.id, a->low_res.id, 0, 0, a->lo_dim.x * 2,
                              a->lo_dim.y * 2, 0, 0,
                              a->lo_dim.x, a->lo_dim.y, GL_COLOR_BUFFER_BIT,
                              GL_LINEAR);

    fbo_bind(&a->low_res_2);
    gl_viewport(0, 0, a->lo_dim.x, a->lo_dim.y);
    gl_disable(GL_DEPTH_TEST);
    gl_clear(GL_COLOR_BUFFER_BIT);

    shdr_bind(&a->dither);
    dither_up(&a->dither,
              (dither){
                .tex = fbo_tex_at(&a->low_res, GL_COLOR_ATTACHMENT0),
                .unit = 0,
                .pal_size = dreamy_haze_size,
                .pal = dreamy_haze
              });

    vao_bind(&a->post);
    gl_draw_arrays(GL_TRIANGLES, 0, 6);

    gl_bind_framebuffer(GL_FRAMEBUFFER, 0);
    gl_viewport(0, 0, a->dim.x, a->dim.y);

    shdr_bind(&a->crt);
    crt_up(&a->crt, (crt){
      .tex = fbo_tex_at(&a->low_res_2, GL_COLOR_ATTACHMENT0),
      .unit = 0,
      .aspect = a->dim.x / a->dim.y,
      .lores = low_res
    });

    vao_bind(&a->post);
    gl_draw_arrays(GL_TRIANGLES, 0, 6);

    gl_enable(GL_BLEND);

    draw_rect((v2){10, 10}, (v2){640, 20 + a->text.size * 6 + 10},
              (v4){0.f, 0.f, 0.f, 0.5f});
    char text_buf[128];
    sprintf_s(text_buf, 128, "&bxyz&r: &b%.2f&r &b%.2f&r &b%.2f", a->cam.pos.x,
              a->cam.pos.y,
              a->cam.pos.z);
    font_draw(&a->text, text_buf, (v2){20, 20}, 0xffffffff, 1, 1.f);
    sprintf_s(text_buf, 128,
              "&bmsp&r(&bt&r/&bf&r/&bd&r): &b%.3f&r/&b%.3f/&b%.3f",
              avg_num_get(&a->mspt),
              avg_num_get(&a->mspf),
              avg_num_get(&a->mspd));
    font_draw(&a->text, text_buf, (v2){20, 20 + a->text.size}, 0xffffffff,
              1, 1.f);
    sprintf_s(text_buf, 128, "&bworld size&r: &b%zu&r/&b%zu",
              a->world->chunks.count,
              a->world->chunks.cap);
    font_draw(&a->text, text_buf, (v2){20, 20 + a->text.size * 2},
              0xffffffff, 1, 1.f);
    sprintf_s(text_buf, 128, "&b# objects&r: &b%zu", arr_len(a->world->objs));
    font_draw(&a->text, text_buf, (v2){20, 20 + a->text.size * 3},
              0xffffffff, 1, 1.f);
    sprintf_s(text_buf, 128, "&bculled&r: &b%.2f%%&r",
              (1.f - (float)a->n_drawn / (float)a->n_close) * 100.f);
    font_draw(&a->text, text_buf, (v2){20, 20 + a->text.size * 4},
              0xffffffff, 1, 1.f);
    sprintf_s(text_buf, 128, "&btris&r: &b%zu&r", a->n_tris);
    font_draw(&a->text, text_buf, (v2){20, 20 + a->text.size * 5},
              0xffffffff, 1, 1.f);

    draw_graph(a, &a->mspf, (v4){1.f, 1.f, 0.f, 1.f}, 1);
    draw_graph(a, &a->mspd, (v4){1.f, 0.f, 1.f, 1.f}, 0);
    draw_graph(a, &a->mspt, (v4){0.f, 1.f, 1.f, 1.f}, 0);

    avg_num_add(&a->mspd, (app_now() - start));

    if (!a->is_mouse_captured) {
      win_draw(&a->win);
    }

    glfw_swap_buffers(a->glfw_win);
    avg_num_add(&a->mspf, (app_now() - start));
    glfw_poll_events();

    arena_reset(&$.temp);
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
  k->dim = (v2){(float)width, (float)height};
  k->lo_dim = (iv2){(int)(low_res * (float)width / (float)height),
                    (int)low_res};
  fbo_resize(&k->low_res, k->lo_dim.x, k->lo_dim.y, 1,
             (u32[]){GL_COLOR_ATTACHMENT0});
  fbo_resize(&k->low_res_2, k->lo_dim.x, k->lo_dim.y, 1,
             (u32[]){GL_COLOR_ATTACHMENT0});
  fbo_resize(&k->main, k->lo_dim.x * 2, k->lo_dim.y * 2, 2,
             (u32[]){GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT});
  k->cam.aspect = (float)width / (float)height;
  gl_viewport(0, 0, width, height);
}

void cursor_pos_callback(GLFWwindow *win, double xpos, double ypos) {
  app *k = glfw_get_window_user_pointer(win);
  k->mouse = (v2){(float)xpos, (float)ypos};
  cam_tick(&k->cam);
}

void
key_callback(GLFWwindow *win, int keycode, int scancode, int action, int mods) {
  app *g = glfw_get_window_user_pointer(win);
  switch (keycode) {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_GRAVE_ACCENT: {
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

  if (action == GLFW_RELEASE) {
    win_rel(&a->win);
  }

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
gl_error_callback(u32 source, u32 type, u32 id, u32 severity, int length,
                  char const *message, void const *user_param) {
  printf("%s\n", message);
}
