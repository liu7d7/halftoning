#pragma once

#include "stb_truetype.h"
#include "gl.h"

typedef struct font_vtx {
  v3f pos;
  v2f uv;
  v4f col;
} font_vtx;

enum font_wt {
  fw_reg,
  fw_bold,
  fw_ita,
  fw_bold_ita,
  fw_n
};

typedef struct font {
  buf vb;
  vao va;

  stbtt_packedchar *chars[fw_n];
  int n_chars;
  font_vtx *vs;
  tex tex;
  float size, ascent[fw_n];
} font;

font font_new(u8 *font_file[fw_n], int n_chars, float size);

float font_get_width(font *f, char const *text, float scale);

void font_draw(font *f, struct app *a, const char *text, v2f pos, uint color,
               int shadow,
               float scale);

shader *font_get_sh(struct app *a);