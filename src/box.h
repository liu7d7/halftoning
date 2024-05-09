#pragma once

#include "typedefs.h"

typedef struct box3 {
  v3f min, max;
} box3;

[[gnu::always_inline]]
inline static box3 box3_new(v3f min, v3f max) {
  return (box3){.min = min, .max = max};
}

[[gnu::always_inline]]
inline static box3 box3_fit(box3 a, box3 b) {
  return box3_new(v3_min(a.min, b.min), v3_max(a.max, b.max));
}

[[gnu::always_inline]]
inline static box3 box3_add(box3 a, v3f b) {
  return box3_new(v3_add(a.min, b), v3_add(a.max, b));
}

struct frustum;
int box3_viewable(box3 b, struct frustum *f);

[[gnu::always_inline]]
inline static bool box3_overlaps(box3 a, box3 b) {
#define overlaps(x_min0, x_min1, x_max0, x_max1) (x_max0 >= x_min1 && x_max1 >= x_min0)
  return overlaps(a.min.x, b.min.x, a.max.x, b.max.x)
         && overlaps(a.min.y, b.min.y, a.max.y, b.max.y)
         && overlaps(a.min.z, b.min.z, a.max.z, b.max.z);
#undef overlaps
}

typedef struct box2 {
  v2f min, max;
} box2;

box2 box2_new(v2f min, v2f max);

bool box2_contains(box2 a, v2f pos);