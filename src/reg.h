#pragma once

#include "body.h"

/*-- a 2d rectangle in space. --*/

typedef struct reg {
  body *sta;
  body **dyn;
} reg;

reg reg_new();

void reg_tick(reg *r);

void reg_clear(reg *r);

bool reg_is_tickable(reg *r);

void reg_add_sta(reg *r, body *o);

void reg_add_sta_v(reg *r, body *o);

void reg_add_dyn(reg *r, body *o);

void reg_add_dyn_v(reg *r, body **o);