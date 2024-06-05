#pragma once

#include "gl.h"
#include "body.h"

#define chunk_size 8
#define chunk_sizef 8.f
#define chunk_qty 8
#define chunk_len (chunk_qty + 1)
static const float chunk_ratio = (float)chunk_size / (float)chunk_qty;

typedef struct ch_vtx {
  v3 pos;
  v3 norm;
  int id;
} ch_vtx;

typedef struct chunk {
  body body;
  ch_vtx data[chunk_len * chunk_len];
  int id;
} chunk;

float chunk_get_y(v3 world_pos);

v3 chunk_get_pos(iv2 pos, int off_x, int off_z);

struct world;
chunk chunk_new(struct world *w, iv2 pos);

shdr *ch_get_sh(draw_src s, cam *c);