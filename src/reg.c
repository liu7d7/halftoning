#include "reg.h"
#include "arr.h"

void reg_tick(reg *r) {
  for (body *s = r->sta, *s_end = arr_end(r->sta); s != s_end; s++) {
    for (body **d = r->dyn, **d_end = arr_end(r->dyn); d != d_end; d++) {
      hit h = body_hit(s, *d);
      if (!h.is_hit) continue;

      body_response(*d, h, sqrtf(s->slip * (*d)->slip));
    }
  }

  for (body **a = r->dyn, **end = arr_end(r->dyn); a != end - 1; a++) {
    for (body **b = a + 1; b != end; b++) {
      hit h = body_hit(*a, *b);
      if (!h.is_hit) continue;

      hit a_hit = h, b_hit = h;
      a_hit.push *= -0.5f, b_hit.push *= 0.5f;

      body_response(*a, a_hit, sqrtf((*a)->slip * (*b)->slip));
      body_response(*b, b_hit, sqrtf((*a)->slip * (*b)->slip));
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

void reg_add_sta(reg *r, body *o) {
  arr_add(&r->sta, o);
}

void reg_add_sta_v(reg *r, body *o) {
  arr_add_bulk(&r->sta, o);
}

void reg_add_dyn(reg *r, body *o) {
  arr_add(&r->dyn, &o);
}

void reg_add_dyn_v(reg *r, body **o) {
  arr_add_bulk(&r->dyn, o);
}

reg reg_new() {
  return (reg){
    .sta = arr_new(body),
    .dyn = arr_new(body*),
  };
}


