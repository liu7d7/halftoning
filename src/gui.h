#pragma once

#include <stddef.h>
#include "typedefs.h"
#include "box.h"
#include "avg.h"

typedef struct win {
  char const *title;
  v2 drag, rsz_drag, pos, dim;
  box2 bar, rsz;
  bool rsz_dragging, dragging;
} win;

void win_draw(win *w);

int win_click(win *w, v2 pos);

win win_new(const char *title, v2 pos, v2 dim);

void win_key(win *w, char c);

void draw_circle(v2 pos, float rad, v4 color);

void win_rel(win *w);

void draw_rect(v2 tl, v2 br, v4 color);

void draw_line_graph(v2 *points, v4 color);