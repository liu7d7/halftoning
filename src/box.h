#pragma once

#include "typedefs.h"

typedef struct box3 {
  v3f min, max;
} box3;

box3 box3_new(v3f min, v3f max);

box3 box3_fit_2(box3 a, box3 b);

bool box3_is_overlap(box3 a, box3 b);

typedef struct box2 {
  v2f min, max;
} box2;

box2 box2_new(v2f min, v2f max);

bool box2_contains(box2 a, v2f pos);