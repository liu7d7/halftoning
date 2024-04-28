#pragma once

#include "gl.h"
#include "body.h"

static const int chunk_size = 8;
static const float chunk_sizef = 8;
static const int chunk_qty = 8;
static const int chunk_len = chunk_qty + 1;
static const float chunk_ratio = (float)chunk_size / (float)chunk_qty;

typedef struct chunk {
  vao vao;
  int n_inds;
  v2i pos;

  body body;
} chunk;

float chunk_get_y(v3f world_pos);

v3f chunk_get_pos(v2i pos, int off_x, int off_z);

chunk chunk_new(v2i pos);