#pragma once

#include "body.h"
#include "gl.h"

/*-- an obj that exists in a game world --*/

typedef enum obj_type {
  ot_hana,
  ot_test,
  ot_tree
} obj_type;

typedef struct hana {
  obj_type type;
} hana;

typedef struct tree {
  obj_type type;
  v3f offset, dir;
  int idx;
  float rot;
  box3 box;
} tree;

typedef struct obj {
  union {
    obj_type type;
    hana hana;
    tree tree;
  };

  int id;
  bool dynamic;
  struct world *world;
  body body;
} obj;

obj hana_new();

obj test_new(v3f pos, v3f vel, float rad);

obj tree_new(v3f pos, v3f dir);

void obj_draw(obj *o, draw_src s, cam *c, float d);

void obj_tick(obj *o);

v3f obj_get_ipos(obj *o, float d);

box3 obj_get_box(obj *o);