#pragma once

#include <pthread.h>
#include "gl.h"
#include "lib/simplex/FastNoiseLite.h"
#include "arr.h"
#include "map.h"
#include "reg.h"
#include "map.h"
#include "chunk.h"
#include "obj.h"

/*-- a 3d world using simplex noise. --*/

#define world_draw_dist 24
#define world_sp_size (world_draw_dist * 2 + 1)

typedef struct world {
  // iv2 -> chunk
  map chunks;
  reg regions[world_sp_size][world_sp_size];

  obj *objs, *objs_tick, *objs_to_add;
  buf vb, ib;
  vao va;
  ch_vtx *vb_cache;
  int *ib_cache;
  bool vb_dirty, ib_dirty;
  iv2 last_chunk_pos;

  pthread_mutex_t draw_lock;
  int _Atomic id;
} world;

// requires an opengl context!
world *world_new(obj player);

iv2 world_get_chunk_pos(v3 world_pos);

void world_tick(world *w, cam *c);

void world_cache(world *w, iv2 cam_to_chunk);

void world_draw(world *w, draw_src s, cam *c, float d);

void world_add_obj(world *w, obj *o);