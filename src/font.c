#include "font.h"
#include "arr.h"
#include "app.h"

font font_new(u8 *font_file, int n_chars, float size) {
  buf b = buf_new(GL_ARRAY_BUFFER);

  font f = {
    .size = size,
    .n_chars = n_chars,
    .chars = arr_new(stbtt_packedchar, n_chars),
    .va = vao_new(&b, NULL, 3, (attrib[]){attr_3f, attr_2f, attr_4f}),
    .vb = b,
    .vs = arr_new(font_vtx, 576)
  };

  stbtt_fontinfo inf;
  if (!stbtt_InitFont(&inf, font_file, 0)) {
    throw_c("font_new: failed to load font");
  }

  stbtt_pack_context ctx;
  u8 *bitmap = malloc(2048 * 2048);
  stbtt_PackBegin(&ctx, bitmap, 2048, 2048, 0, 1, NULL);
  stbtt_PackSetOversampling(&ctx, 8, 8);
  stbtt_PackFontRange(&ctx, font_file, 0, size, 32, n_chars, f.chars);
  stbtt_PackEnd(&ctx);

  int asc;
  stbtt_GetFontVMetrics(&inf, &asc, nullptr, nullptr);
  f.ascent = (float)asc * stbtt_ScaleForMappingEmToPixels(&inf, size);
  f.tex = tex_new(tex_spec_r8v(2048, 2048, GL_LINEAR, bitmap));

  free(bitmap);
  free(font_file);

  return f;
}

void
font_draw(font *f, app *a, const char *text, v2f pos, uint color, int shadow,
          float scale) {
  arr_clear(f->vs);
  float dx = pos.x;
  float dy = pos.y + f->ascent * scale - (6 * (float)f->size / 40.f * scale);
  float r = (float)((color >> 16) & 0xFF) / 255.0f;
  float g = (float)((color >> 8) & 0xFF) / 255.0f;
  float b = (float)(color & 0xFF) / 255.0f;
  float alpha = (float)((color >> 24) & 0xFF) / 255.0f;

  for (char const *c = text; *c; c++) {
    char cur = *c;
    if (cur < 32 || cur > 32 + f->n_chars) cur = ' ';
    stbtt_packedchar pc = f->chars[cur - 32];
    float dxs = dx + pc.xoff * scale;
    float dys = dy + pc.yoff * scale;
    float dx1s = dx + pc.xoff2 * scale;
    float dy1s = dy + pc.yoff2 * scale;

    v4f col;
    if (shadow) {
      col = (v4f){r * 0.25f, g * 0.25f, b * 0.25f, alpha};
      arr_add(&f->vs, &(font_vtx){.pos = {dxs + 1, dys + 1, 1}, .uv = {
        (float)pc.x0 / 2048.f, (float)pc.y0 / 2048.f}, .col = col});
      arr_add(&f->vs, &(font_vtx){.pos = {dx1s + 1, dys + 1, 1}, .uv = {
        (float)pc.x1 / 2048.f, (float)pc.y0 / 2048.f}, .col = col});
      arr_add(&f->vs, &(font_vtx){.pos = {dx1s + 1, dy1s + 1, 1}, .uv = {
        (float)pc.x1 / 2048.f, (float)pc.y1 / 2048.f}, .col = col});
      arr_add(&f->vs, &(font_vtx){.pos = {dx1s + 1, dy1s + 1, 1}, .uv = {
        (float)pc.x1 / 2048.f, (float)pc.y1 / 2048.f}, .col = col});
      arr_add(&f->vs, &(font_vtx){.pos = {dxs + 1, dy1s + 1, 1}, .uv = {
        (float)pc.x0 / 2048.f, (float)pc.y1 / 2048.f}, .col = col});
      arr_add(&f->vs, &(font_vtx){.pos = {dxs + 1, dys + 1, 1}, .uv = {
        (float)pc.x0 / 2048.f, (float)pc.y0 / 2048.f}, .col = col});
    }

    col = (v4f){r, g, b, alpha};
    arr_add(&f->vs,
            &(font_vtx){.pos = {dxs, dys, 1}, .uv = {(float)pc.x0 / 2048.f,
                                                     (float)pc.y0 /
                                                     2048.f}, .col = col});
    arr_add(&f->vs,
            &(font_vtx){.pos = {dx1s, dys, 1}, .uv = {(float)pc.x1 / 2048.f,
                                                      (float)pc.y0 /
                                                      2048.f}, .col = col});
    arr_add(&f->vs,
            &(font_vtx){.pos = {dx1s, dy1s, 1}, .uv = {(float)pc.x1 / 2048.f,
                                                       (float)pc.y1 /
                                                       2048.f}, .col = col});
    arr_add(&f->vs,
            &(font_vtx){.pos = {dx1s, dy1s, 1}, .uv = {(float)pc.x1 / 2048.f,
                                                       (float)pc.y1 /
                                                       2048.f}, .col = col});
    arr_add(&f->vs,
            &(font_vtx){.pos = {dxs, dy1s, 1}, .uv = {(float)pc.x0 / 2048.f,
                                                      (float)pc.y1 /
                                                      2048.f}, .col = col});
    arr_add(&f->vs,
            &(font_vtx){.pos = {dxs, dys, 1}, .uv = {(float)pc.x0 / 2048.f,
                                                     (float)pc.y0 /
                                                     2048.f}, .col = col});

    dx += pc.xadvance * scale;
  }

  buf_data_n(&f->vb, GL_DYNAMIC_DRAW, sizeof(font_vtx), arr_len(f->vs), f->vs);

  tex_bind(&f->tex, 0);
  font_get_shader(a);
  vao_bind(&f->va);
  gl_draw_arrays(GL_TRIANGLES, 0, arr_len(f->vs));
}

float font_get_width(font *f, char const *text, float scale) {
  float dx = 0;

  for (char const *c = text; *c; c++) {
    char cur = *c;
    if (cur < 32 || cur > 32 + f->n_chars) cur = ' ';
    stbtt_packedchar pc = f->chars[cur - 32];
    dx += pc.xadvance * scale;
  }

  return dx;
}

shader *font_get_shader(struct app *a) {
  static shader *sh = NULL;
  if (!sh) {
    sh = objdup(shader_new(2, (shader_spec[]){
      {GL_VERTEX_SHADER,   "res/font.vsh"},
      {GL_FRAGMENT_SHADER, "res/font.fsh"}
    }));
  }

  shader_mat4(sh, "u_proj", m4_ortho(0, a->dim.x, a->dim.y, 0, -5, 5));

  return sh;
}
