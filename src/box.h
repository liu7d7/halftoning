#pragma once

#include "typedefs.h"

typedef struct box3 {
  v3 min, max;
} box3;

[[gnu::always_inline]]
inline static box3 box3_new(v3 min, v3 max) {
  return (box3){.min = min, .max = max};
}

[[gnu::always_inline]]
inline static box3 box3_fit(box3 a, box3 b) {
  return box3_new(v3_min(a.min, b.min), v3_max(a.max, b.max));
}

[[gnu::always_inline]]
inline static float box3_dist(box3 a, v3 b) {
  v3 verts[8] = {
    {a.min.x, a.min.y, a.min.z},
    {a.max.x, a.min.y, a.min.z},
    {a.min.x, a.max.y, a.min.z},
    {a.max.x, a.max.y, a.min.z},
    {a.min.x, a.min.y, a.max.z},
    {a.max.x, a.min.y, a.max.z},
    {a.min.x, a.max.y, a.max.z},
    {a.max.x, a.max.y, a.max.z},
  };

  float min_dist_sq = 1e20f;
  for (int i = 0; i < 8; i++) {
    v3 delta = v3_sub(verts[i], b);
    min_dist_sq = min(min_dist_sq, v3_dot(delta, delta));
  }

  return sqrtf(min_dist_sq);
}

[[gnu::always_inline]]
inline static box3 box3_add(box3 a, v3 b) {
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
  v2 min, max;
} box2;

box2 box2_new(v2 min, v2 max);

bool box2_contains(box2 a, v2 pos);