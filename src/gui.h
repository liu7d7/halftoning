#pragma once

#include <stddef.h>
#include "typedefs.h"
#include "box.h"
#include "avg.h"

typedef enum widget_type {
  wt_info,
  wt_slider,
  wt_input_box,
} widget_type;

typedef char const *(*info_cb)(void *user_data);

typedef struct info {
  widget_type type;
  void *user_data;
  info_cb cb;
} info;

info info_new(void *user_data, info_cb cb);

typedef void (*slider_cb)(void *user_data, double val);

typedef struct slider {
  widget_type type;
  void *user_data;
  double min, max, val;
  slider_cb cb;
} slider;

slider
slider_new(float min, float max, float val, void *user_data, slider_cb cb);

typedef void (*input_box_cb)(void *user_data, char *val);

typedef struct input_box {
  widget_type type;
  void *user_data;
  char *val;
  input_box_cb cb;
} input_box;

input_box input_box_new(char *val, void *user_data, input_box_cb cb);

typedef union widget {
  widget_type type;
  info info;
  slider slider;
  input_box in_box;
} widget;

struct app;

void widget_draw(widget *w);

void widget_click(widget *w, v2 pos);

void widget_key(widget *w, char c);

typedef struct win {
  char const *title;
  widget *widgets;
  v2 drag, rsz_drag, pos, dim;
  box2 bar, rsz;
} win;

void win_draw(win *w);

int win_click(win *w, v2 pos);

win win_new(const char *title, widget *widgets, v2 pos, v2 dim);

void win_key(win *w, char c);

void draw_circle(v2 pos, float rad, v4 color);

void draw_rect(v2 tl, v2 br, v4 color);

void draw_line_graph(v2 *points, v4 color);