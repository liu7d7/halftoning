#include "box.h"

box2 box2_new(v2f min, v2f max) {
  return (box2){.min = min, .max = max};
}

bool box2_contains(box2 a, v2f pos) {
#define contains(x_min, x_max, x_pos) (x_pos >= x_min && x_pos <= x_max)
  return contains(a.min.x, a.max.x, pos.x) && contains(a.min.y, a.max.y, pos.y);
#undef contains
}