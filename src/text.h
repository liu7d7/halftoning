#pragma once

#include "stb_truetype.h"
#include "gl.h"

typedef struct font_vtx {
  v3 pos;
  v2 uv;
  v4 col;
} font_vtx;

enum font_wt {
  fw_reg,
  fw_bold,
  fw_ita,
  fw_bold_ita,
  fw_n
};

typedef struct text {
  buf vb;
  vao va;

  stbtt_packedchar *chars[fw_n];
  int n_chars;
  font_vtx *vs;
  tex tex;
  float size, ascent[fw_n];
} text;

text font_new(u8 *font_file[fw_n], int n_chars, float size);

float font_get_width(text *f, char const *text, float scale);

void font_draw(text *f, const char *text, v2 pos, u32 color,
               int shadow,
               float scale);

shdr *font_get_sh();