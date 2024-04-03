#include "reg.h"
#include "arr.h"

void obj_response(obj *o, hit h) {
  obj_move(o, v3_mul(h.norm, h.push + 0.0001f));
  auto vel = obj_get_vel(o);
  float vel_len = v3_len(*vel);
  v3f vel_normed = v3_div(*vel, vel_len);
  v3f undesired_motion = v3_mul(h.norm, v3_dot(vel_normed, h.norm));
  v3f desired_motion = v3_sub(vel_normed, undesired_motion);
  *vel = v3_mul(desired_motion, vel_len);
}

void reg_tick(reg *r) {
  for (obj *s = r->sta, *s_end = arr_end(r->sta); s != s_end; s++) {
    for (obj **d = r->dyn, **d_end = arr_end(r->dyn); d != d_end; d++) {
      hit h = obj_hit(s, *d);
      if (!h.is_hit) continue;

      obj_response(*d, h);
    }
  }

  for (obj **a = r->dyn, **end = arr_end(r->dyn); a != end - 1; a++) {
    for (obj **b = a + 1; b != end; b++) {
      hit h = obj_hit(*a, *b);
      if (!h.is_hit) continue;

      hit a_hit = h, b_hit = h;
      a_hit.push *= -0.5f, b_hit.push *= 0.5f;

      obj_response(*a, a_hit);
      obj_response(*b, b_hit);
    }
  }
}

void reg_clear(reg *r) {
  arr_clear(r->dyn);
  arr_clear(r->sta);
}

bool reg_is_tickable(reg *r) {
  return !arr_is_empty(r->dyn);
}

void reg_add_sta(reg *r, obj *o) {
  arr_add(&r->sta, o);
}

void reg_add_sta_v(reg *r, obj *o) {
  arr_add_bulk(&r->sta, o);
}

void reg_add_dyn(reg *r, obj *o) {
  arr_add(&r->dyn, &o);
}

void reg_add_dyn_v(reg *r, obj **o) {
  arr_add_bulk(&r->dyn, o);
}

reg reg_new() {
  return (reg){
    .sta = arr_new(obj, 4),
    .dyn = arr_new(obj*, 4),
  };
}


