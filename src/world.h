#pragma once

#include "gl.h"
#include "lib/simplex/FastNoiseLite.h"
#include "dyn_arr.h"
#include "map.h"

static const int chunk_size = 32;
static const int chunk_qty = 16;
static const int chunk_len = chunk_qty + 1;
static const int chunk_ratio = chunk_size / chunk_qty;

struct chunk {
  struct vao vao;
  int n_inds;
  ivec2 pos;
};

struct chunk_vtx {
  vec3 pos;
  vec3 norm;
  vec2 padding;
};

float chunk_get_y(vec3 const world_pos);

void chunk_get_pos(ivec2 const pos, int off_x, int off_z, vec3 out);

struct chunk chunk(ivec2 pos);

struct world {
  // ivec2 -> chunk
  struct map chunks;
};

// requires an opengl context!
struct world world();

void world_get_chunk_pos(vec3 const world_pos, ivec2 out);

void world_draw(struct world* w, struct cam* c);