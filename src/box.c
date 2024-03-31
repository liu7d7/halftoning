#include "box.h"

bool box3_is_overlap(box3 a, box3 b) {
#define overlaps(x_min0, x_min1, x_max0, x_max1) (x_max0 >= x_min1 && x_max1 >= x_min0)
  return overlaps(a.min.x, b.min.x, a.max.x, b.max.x)
         && overlaps(a.min.y, b.min.y, a.max.y, b.max.y)
         && overlaps(a.min.z, b.min.z, a.max.z, b.max.z);
#undef overlaps
}

box3 box3_new(v3f min, v3f max) {
  return (box3){.min = min, .max = max};
}

box2 box2_new(v2f min, v2f max) {
  return (box2){.min = min, .max = max};
}

bool box2_contains(box2 a, v2f pos) {
#define contains(x_min, x_max, x_pos) (x_pos >= x_min && x_pos <= x_max)
  return contains(a.min.x, a.max.x, pos.x) && contains(a.min.y, a.max.y, pos.y);
#undef contains
}

box3 box3_fit_2(box3 a, box3 b) {
  return box3_new(v3_min(a.min, b.min), v3_max(a.max, b.max));
}
