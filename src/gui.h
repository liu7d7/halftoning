#pragma once

#include <stddef.h>
#include "typedefs.h"
#include "box.h"

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

void widget_draw(widget *w, struct app *a);

void widget_click(widget *w, v2f pos);

void widget_key(widget *w, char c);

typedef struct win {
  char const *title;
  widget *widgets;
  v2f drag, rsz_drag, pos, dim;
  box2 bar, rsz;
} win;

void win_draw(win *w, struct app *a);

int win_click(win *w, v2f pos);

win win_new(const char *title, widget *widgets, v2f pos, v2f dim);

void win_key(win *w, char c);

void draw_circle(struct app *a, v2f pos, float rad, v4f color);

void draw_rect(struct app *a, v2f tl, v2f br, v4f color);

void draw_line_graph(struct app *a, v2f points, v4f color);