#pragma once

#include <pthread.h>
#include "gl.h"
#include "lib/simplex/FastNoiseLite.h"
#include "arr.h"
#include "map.h"
#include "reg.h"
#include "lmap.h"

/*-- a 3d world using simplex noise. --*/

static const int chunk_size = 8;
static const int chunk_qty = 8;
static const int chunk_len = chunk_qty + 1;
static const float chunk_ratio = (float)chunk_size / (float)chunk_qty;

typedef struct chunk {
  vao vao;
  int n_inds;
  v2i pos;

  obj obj;
} chunk;

typedef struct chunk_vtx {
  v3f pos;
  v3f norm;
  v2f padding;
} chunk_vtx;

float chunk_get_y(v3f world_pos);

v3f chunk_get_pos(v2i pos, int off_x, int off_z);

chunk chunk_new(v2i pos);

#define world_draw_dist 24
#define world_sp_size (world_draw_dist * 2 + 1)

typedef struct world {
  // v2i -> chunk
  lmap chunks;
  reg regions[world_sp_size][world_sp_size];

  obj *objs_tick;
  obj *objs_draw;
  pthread_mutex_t render_lock;
} world;

// requires an opengl context!
world world_new();

v2i world_get_chunk_pos(v3f world_pos);

void world_tick(world *w, cam *c);

void world_draw(world *w, cam *c, float d);