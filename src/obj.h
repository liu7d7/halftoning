#pragma once

#include "body.h"

/*-- an obj that exists in a game world --*/

typedef enum obj_type {
  ot_hana,
  ot_test
} obj_type;

typedef struct hana {
  obj_type type;
} hana;

typedef struct obj {
  union {
    obj_type type;
    hana hana;
  };

  int id;
  struct world *world;
  body body;
} obj;

obj hana_new();

obj test_new(v3f pos, v3f vel, float rad);

void obj_draw(obj *o, cam *c, float d);

void obj_tick(obj *o);

v3f obj_get_ipos(obj *o, float d);