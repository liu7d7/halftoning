#pragma once

#include "typedefs.h"
#include "gl.h"
#include "box.h"

typedef enum obj_type {
  ot_tri = 1 << 0,
  ot_mesh = 1 << 1,
  ot_cap = 1 << 2,
  ot_ball = 1 << 3
} obj_type;

typedef struct tri {
  obj_type type;
  v3f pos[3];
  v3f norm;
  bool is_double_sided;
} tri;

tri tri_new(v3f a, v3f b, v3f c, v3f norm, bool double_sided);

typedef struct cap {
  obj_type type;
  v3f pos, prev_pos, vel, norm;
  float ext, rad;
} cap;

cap cap_new(v3f pos, v3f norm, float rad, float ext);

typedef struct ball {
  obj_type type;
  v3f pos, prev_pos, vel;
  float rad;
} ball;

ball ball_new(v3f pos, float rad);

typedef struct tmesh {
  obj_type type;
  union obj *tris;
  box3 box;
} tmesh;

tmesh tmesh_new(tri *tris);

typedef union obj {
  obj_type type;

  tri t;
  cap c;
  ball b;
  tmesh m;

  float pad[16];
} obj;

typedef struct hit {
  bool is_hit;
  v3f norm;
  float push;
} hit;

static hit hit_miss = {
  .is_hit = 0
};

inline static bool obj_is_dyn(obj *o) {
  return o->type == ot_cap || o->type == ot_ball;
}

hit obj_hit(obj *a, obj *b);

void obj_tick(obj *o, float t);

void obj_move(obj *o, v3f d);

void obj_draw(obj *o, cam *c, float d);

v3f *obj_get_pos(obj *o);

v3f *obj_get_prev_pos(obj *o);

v3f *obj_get_vel(obj *o);

box3 obj_get_box(obj *o);

bool obj_is_quick_intersecting(obj *a, obj *b);