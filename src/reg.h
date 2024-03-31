#pragma once

#include "obj.h"

/*-- a 2d rectangle in space. --*/

typedef struct reg {
  obj *sta;
  obj **dyn;
  box3 box;
} reg;

reg reg_new();

void reg_tick(reg *r);

void reg_clear(reg *r);

bool reg_is_tickable(reg *r);

void reg_add_sta(reg *r, obj *o);

void reg_add_sta_v(reg *r, obj *o);

void reg_add_dyn(reg *r, obj *o);

void reg_add_dyn_v(reg *r, obj **o);