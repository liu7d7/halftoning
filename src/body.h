#pragma once

#include "typedefs.h"
#include "box.h"

/*-- a physics body in 3d space. --*/

typedef enum body_type {
  bt_mesh = 1 << 0,
  bt_cap = 1 << 1,
  bt_ball = 1 << 2
} body_type;

typedef struct tri {
  v3f pos[3];
  v3f norm;
  box3 box;
} tri;

tri tri_new(v3f a, v3f b, v3f c, v3f norm);

typedef struct cap {
  body_type type;
  v3f norm;
  float ext, rad;
} cap;

cap cap_new(v3f norm, float rad, float ext);

typedef struct ball {
  body_type type;
  float rad;
} ball;

ball ball_new(float rad);

typedef struct tmesh {
  body_type type;
  tri *tris;
  box3 box;
} tmesh;

struct obj_vtx;

tmesh tmesh_new(tri *tris);
tmesh tmesh_new_vi(struct obj_vtx *verts, int *inds);

tmesh tmesh_add(tmesh *orig, v3f pos);

typedef struct body {
  union {
    body_type type;

    cap cap;
    ball ball;
    tmesh mesh;
  };

  float slip;
  bool on_ground;
  v3f pos, prev_pos, vel;
} body;

typedef struct hit {
  bool is_hit;
  v3f norm;
  float push;
} hit;

hit hit_inv(hit h);

static hit hit_miss = {
  .is_hit = 0
};

inline static bool body_is_dyn(body *o) {
  return o->type == bt_cap || o->type == bt_ball;
}

void body_response(body *o, hit h, float slip);

v3f body_get_ipos(body *o, float d);

hit body_hit(body *a, body *b);

void body_tick(body *o, float t);

box3 body_get_box(body *o);

[[gnu::always_inline]]
inline static bool body_is_hit_plausible(body *a, body *b) {
  return box3_overlaps(body_get_box(a), body_get_box(b));
}