#include "gui.h"
#include "app.h"

void draw_circle(v2 pos, float rad, v4 color) {
  static shdr *sh = NULL;
  static buf *vb = NULL;
  static vao *va = NULL;
  if (!sh) {
    sh = _new_(shdr_new(2, (shdr_s[]){
      {GL_VERTEX_SHADER,   "res/circle.vsh"},
      {GL_FRAGMENT_SHADER, "res/circle.fsh"}}));

    vb = _new_(buf_new(GL_ARRAY_BUFFER));

    va = _new_(vao_new(vb, NULL, 1, (attrib[]){attr_2f}));
  }

  float aa = min(rad / 10.f, 3);
  float real = rad + aa;

  v2 v00 = v2_add(pos, (v2){-real, -real}), v01 = v2_add(pos,
                                                         (v2){-real, real}),
    v11 = v2_add(pos, (v2){real, real}), v10 = v2_add(pos, (v2){real, -real});
  buf_data_n(vb, GL_DYNAMIC_DRAW, sizeof(v2), 6,
             (v2[]){v00, v01, v11, v11, v10, v00});

  shdr_bind(sh);
  shdr_4f(sh, "u_color", color);
  shdr_1f(sh, "u_rad", rad);
  shdr_2f(sh, "u_center", pos);
  shdr_m4f(sh, "u_proj",
           m4_ortho(0, $.dim.x, $.dim.y, 0, -1.f, 1.f));
  vao_bind(va);
  gl_draw_arrays(GL_TRIANGLES, 0, 6);
}

void draw_rect(v2 tl, v2 br, v4 color) {
  static shdr *sh = NULL;
  static buf *vb = NULL;
  static vao *va = NULL;
  if (!sh) {
    sh = _new_(shdr_new(2, (shdr_s[]){
      {GL_VERTEX_SHADER,   "res/rect.vsh"},
      {GL_FRAGMENT_SHADER, "res/rect.fsh"}}));

    vb = _new_(buf_new(GL_ARRAY_BUFFER));

    va = _new_(vao_new(vb, NULL, 1, (attrib[]){attr_2f}));
  }

  v2 v00 = (v2){tl.x, tl.y}, v01 = (v2){tl.x, br.y}, v11 = (v2){br.x, br.y},
    v10 = (v2){br.x, tl.y};
  buf_data_n(vb, GL_DYNAMIC_DRAW, sizeof(v2), 6,
             (v2[]){v00, v01, v11, v11, v10, v00});

  shdr_bind(sh);
  shdr_4f(sh, "u_color", color);
  shdr_m4f(sh, "u_proj",
           m4_ortho(0, $.dim.x, $.dim.y, 0, -1.f, 1.f));
  vao_bind(va);
  gl_draw_arrays(GL_TRIANGLES, 0, 6);
}

void win_update_bounds(win *w) {
  w->dim = v2_max(w->dim, (v2){font_get_width(&$.text, w->title, 1) + 12,
                               $.text.size + 12});
  w->bar = box2_new(w->pos, v2_add(w->pos, (v2){w->dim.x, $.text.size + 12}));
  w->rsz = box2_new(v2_sub(v2_add(w->pos, w->dim), (v2){30, 30}),
                    v2_sub(v2_add(w->pos, w->dim), (v2){6, 6}));
}

void win_draw(win *w) {
  v4 const bg_color = (v4){0, 0, 0, 0.6f};
  v4 const highlight = (v4){1, 0, 1, 0.8f};
  if (w->rsz_dragging) {
    w->dim = v2_add(w->dim, v2_sub($.mouse, w->rsz_drag));
    w->rsz_drag = $.mouse;
  }

  if (w->dragging) {
    w->pos = v2_add(w->pos, v2_sub($.mouse, w->drag));
    w->drag = $.mouse;
  }

  win_update_bounds(w);

  draw_rect(w->pos, v2_add(w->pos, w->dim), bg_color);
  v4 rsz_color = (box2_contains(w->rsz, $.mouse) || w->rsz_dragging) ? highlight
                                                                     : bg_color;
  draw_rect(v2_sub(v2_add(w->pos, w->dim), (v2){30, 30}),
            v2_sub(v2_add(w->pos, w->dim), (v2){6, 6}), rsz_color);
  font_draw(&$.text, w->title, v2_add(w->pos, (v2){6, 6}), 0xffffffff, 1,
            1.f);
}

int win_click(win *w, v2 pos) {
  win_update_bounds(w);

  if (box2_contains(w->rsz, pos)) {
    w->rsz_drag = pos;
    w->rsz_dragging = 1;
    return 1;
  }

  if (box2_contains(w->bar, pos)) {
    w->drag = pos;
    w->dragging = 1;
    return 1;
  }

  if (box2_contains((box2){w->pos, v2_add(w->pos, w->dim)}, pos)) {
    return 1;
  }

  return 0;
}

void win_key(win *w, char c) {

}

win win_new(const char *title, v2 pos, v2 dim) {
  return (win){
    .title = title,
    .drag = v2_zero,
    .pos = pos,
    .dim = dim
  };
}

void draw_line_graph(v2 *points, v4 color) {
  static shdr *sh;
  static vao *va;
  static buf *vb;
  if (!sh) {
    sh = _new_(shdr_new(2, (shdr_s[]){
      {GL_VERTEX_SHADER,   "res/line.vsh"},
      {GL_FRAGMENT_SHADER, "res/line.fsh"},
    }));

    vb = _new_(buf_new(GL_ARRAY_BUFFER));

    va = _new_(vao_new(vb, NULL, 1, (attrib[]){attr_2f}));
  }

  buf_data_n(vb, GL_DYNAMIC_DRAW, sizeof(v2), arr_len(points), points);

  shdr_bind(sh);
  shdr_4f(sh, "u_color", color);
  shdr_m4f(sh, "u_proj", m4_ortho(0, $.dim.x, $.dim.y, 0, -1.f, 1.f));
  vao_bind(va);
  gl_draw_arrays(GL_LINE_STRIP, 0, arr_len(points));
}

void win_rel(win *w) {
  w->rsz_dragging = 0;
  w->dragging = 0;
}
