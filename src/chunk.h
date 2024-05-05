#pragma once

#include "gl.h"
#include "body.h"

#define chunk_size 8
#define chunk_sizef 8.f
#define chunk_qty 8
#define chunk_len (chunk_qty + 1)
static const float chunk_ratio = (float)chunk_size / (float)chunk_qty;

typedef struct chunk {
  body body;
  obj_vtx data[chunk_len * chunk_len];
} chunk;

float chunk_get_y(v3f world_pos);

v3f chunk_get_pos(v2i pos, int off_x, int off_z);

struct world;
chunk chunk_new(struct world *w, v2i pos);