#pragma once

#include "stb_truetype.h"
#include "gl.h"

typedef struct font_vtx {
  v3f pos;
  v2f uv;
  v4f col;
} font_vtx;

typedef struct font {
  buf vb;
  vao va;

  stbtt_packedchar *chars;
  int n_chars;
  font_vtx *vs;
  tex tex;
  float size, ascent;
} font;

font font_new(u8 *font_file, int n_chars, float size);

float font_get_width(font *f, char const *text, float scale);

void font_draw(font *f, struct app *a, const char *text, v2f pos, uint color,
               int shadow,
               float scale);

shader *font_get_shader(struct app *a);