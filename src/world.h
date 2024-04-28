#pragma once

#include <pthread.h>
#include "gl.h"
#include "lib/simplex/FastNoiseLite.h"
#include "arr.h"
#include "map.h"
#include "reg.h"
#include "lmap.h"
#include "chunk.h"
#include "obj.h"

/*-- a 3d world using simplex noise. --*/

#define world_draw_dist 24
#define world_sp_size (24 * 2 + 1)

typedef struct world {
  // v2i -> chunk
  lmap chunks;
  reg regions[world_sp_size][world_sp_size];

  obj *objs, *objs_tick, *objs_to_add;

  pthread_mutex_t draw_lock;
} world;

// requires an opengl context!
world *world_new(obj player);

v2i world_get_chunk_pos(v3f world_pos);

void world_tick(world *w, cam *c);

void world_draw(world *w, cam *c, float d);

void world_add_obj(world *w, obj *o);