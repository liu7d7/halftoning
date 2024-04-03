#include "gui.h"
#include "app.h"

void draw_circle(app *a, v2f pos, float rad, v4f color) {
  static shader *sh = NULL;
  static buf *vb = NULL;
  static vao *va = NULL;
  if (!sh) {
    sh = objdup(shader_new(2, (shader_spec[]){
      {GL_VERTEX_SHADER,   "res/circle.vsh"},
      {GL_FRAGMENT_SHADER, "res/circle.fsh"}}));

    vb = objdup(buf_new(GL_ARRAY_BUFFER));

    va = objdup(vao_new(vb, NULL, 1, (attrib[]){attr_2f}));
  }

  float aa = min(rad / 10.f, 3);
  float real = rad + aa;

  v2f v00 = v2_add(pos, (v2f){-real, -real}), v01 = v2_add(pos,
                                                           (v2f){-real, real}),
    v11 = v2_add(pos, (v2f){real, real}), v10 = v2_add(pos, (v2f){real, -real});
  buf_data_n(vb, GL_DYNAMIC_DRAW, sizeof(v2f), 6,
             (v2f[]){v00, v01, v11, v11, v10, v00});

  shader_bind(sh);
  shader_vec4(sh, "u_color", color);
  shader_float(sh, "u_rad", rad);
  shader_vec2(sh, "u_center", pos);
  shader_mat4(sh, "u_proj",
              m4_ortho(0, a->dim.x, a->dim.y, 0, -1.f, 1.f));
  vao_bind(va);
  gl_draw_arrays(GL_TRIANGLES, 0, 6);
}

void draw_rect(app *a, v2f tl, v2f br, v4f color) {
  static shader *sh = NULL;
  static buf *vb = NULL;
  static vao *va = NULL;
  if (!sh) {
    sh = objdup(shader_new(2, (shader_spec[]){
      {GL_VERTEX_SHADER,   "res/rect.vsh"},
      {GL_FRAGMENT_SHADER, "res/rect.fsh"}}));

    vb = objdup(buf_new(GL_ARRAY_BUFFER));

    va = objdup(vao_new(vb, NULL, 1, (attrib[]){attr_2f}));
  }

  v2f v00 = (v2f){tl.x, tl.y}, v01 = (v2f){tl.x, br.y}, v11 = (v2f){br.x, br.y},
    v10 = (v2f){br.x, tl.y};
  buf_data_n(vb, GL_DYNAMIC_DRAW, sizeof(v2f), 6,
             (v2f[]){v00, v01, v11, v11, v10, v00});

  shader_bind(sh);
  shader_vec4(sh, "u_color", color);
  shader_mat4(sh, "u_proj",
              m4_ortho(0, a->dim.x, a->dim.y, 0, -1.f, 1.f));
  vao_bind(va);
  gl_draw_arrays(GL_TRIANGLES, 0, 6);
}

void win_update_bounds(win *w, app *a) {
  w->dim = v2_max(w->dim, (v2f){font_get_width(&a->font, w->title, 1) + 12,
                                a->font.size + 12});
  w->bar = box2_new(w->pos, v2_add(w->pos, (v2f){w->dim.x, a->font.size + 12}));
  w->rsz = box2_new(v2_sub(v2_add(w->pos, w->dim), (v2f){30, 30}),
                    v2_sub(v2_add(w->pos, w->dim), (v2f){6, 6}));
}

void win_draw(win *w, app *a) {
  win_update_bounds(w, a);

  v4f const bg_color = (v4f){0, 0, 0, 0.6f};
  v4f const highlight = (v4f){1, 0, 1, 0.8f};
  draw_rect(a, w->pos, v2_add(w->pos, w->dim), bg_color);
  v4f rsz_color = box2_contains(w->rsz, a->mouse) ? highlight : bg_color;
  draw_rect(a, v2_sub(v2_add(w->pos, w->dim), (v2f){30, 30}),
            v2_sub(v2_add(w->pos, w->dim), (v2f){6, 6}), rsz_color);
  font_draw(&a->font, a, w->title, v2_add(w->pos, (v2f){6, 6}), 0xffffffff, 1,
            1.f);
}

int win_click(win *w, v2f pos) {
  if (box2_contains(w->rsz, pos)) {
//    w->rsz_drag =
  }
}

void widget_draw(widget *w, app *a) {

}

slider
slider_new(float min, float max, float val, void *user_data, slider_cb cb) {
  return (slider){
    .type = wt_slider,
    .min = min, .max = max, .val = val,
    .cb = cb,
    .user_data = user_data
  };
}

info info_new(void *user_data, info_cb cb) {
  return (info){
    .type = wt_info,
    .cb = cb,
    .user_data = user_data
  };
}

input_box input_box_new(char *val, void *user_data, input_box_cb cb) {
  return (input_box){
    .type = wt_input_box,
    .user_data = user_data,
    .val = val,
    .cb = cb
  };
}

void win_lose_focus(win *w) {

}

void win_key(win *w, char c) {

}

win win_new(const char *title, widget *widgets, v2f pos, v2f dim) {
  return (win){
    .title = title,
    .widgets = widgets,
    .drag = v2_zero,
    .pos = pos,
    .dim = dim
  };
}
