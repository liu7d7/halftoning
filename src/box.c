#include "box.h"
#include "gl.h"

box2 box2_new(v2 min, v2 max) {
  return (box2){.min = min, .max = max};
}

bool box2_contains(box2 a, v2 pos) {
#define contains(x_min, x_max, x_pos) (x_pos >= x_min && x_pos <= x_max)
  return contains(a.min.x, a.max.x, pos.x) && contains(a.min.y, a.max.y, pos.y);
#undef contains
}

int box3_on_forward_plane(v3 ext, v3 pos, plane p) {
  const float r = ext.x * fabsf(p.norm.x) + ext.y * fabsf(p.norm.y) +
                  ext.z * fabsf(p.norm.z);

  return -r <= plane_sdf(p, pos);
}

int box3_viewable(box3 b, struct frustum *f) {
  v3 ext = v3_mul(v3_sub(b.max, b.min), .5f);
  v3 pos = v3_mul(v3_add(b.max, b.min), .5f);
  return box3_on_forward_plane(ext, pos, f->left)
         && box3_on_forward_plane(ext, pos, f->right)
         && box3_on_forward_plane(ext, pos, f->far)
         && box3_on_forward_plane(ext, pos, f->near)
         && box3_on_forward_plane(ext, pos, f->top)
         && box3_on_forward_plane(ext, pos, f->bottom);
}
