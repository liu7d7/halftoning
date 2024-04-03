#include "obj.h"
#include "typedefs.h"
#include "err.h"
#include "arr.h"
#include "box.h"
#include <stddef.h>

hit hit_b(obj *a, obj *b) {
  float len = a->b.rad + b->b.rad;
  float dist = v3_dist(a->b.pos, b->b.pos);
  if (dist > len) {
    return hit_miss;
  }

  float push = len - dist;
  v3f norm = v3_normed(v3_sub(b->b.pos, a->b.pos));

  return (hit){
    .is_hit = 1,
    .norm = norm,
    .push = push
  };
}

v3f point_line_closest(v3f a, v3f b, v3f p) {
  v3f ab = v3_sub(b, a);
  // projection: u dot v / len(u)^2 * v
  float t = v3_dot(v3_sub(p, a), ab) / v3_dot(ab, ab);
  return v3_add(a, v3_mul(ab, clamp(t, 0.f, 1.f)));
}

hit hit_tb(obj *tri_obj, obj *ball_obj) {
  tri t = tri_obj->t;
  ball b = ball_obj->b;
  float dist = v3_dot(v3_sub(b.pos, t.pos[0]), t.norm);
  if (!t.is_double_sided && dist < 0) return hit_miss;
  if (dist < -b.rad || dist > b.rad) return hit_miss;

  v3f p0 = v3_sub(b.pos, v3_mul(t.norm, dist));
  v3f c0 = v3_cross(v3_sub(p0, t.pos[0]), v3_sub(t.pos[1], t.pos[0]));
  v3f c1 = v3_cross(v3_sub(p0, t.pos[1]), v3_sub(t.pos[2], t.pos[1]));
  v3f c2 = v3_cross(v3_sub(p0, t.pos[2]), v3_sub(t.pos[0], t.pos[2]));

  int inside = v3_dot(c0, t.norm) <= 0 && v3_dot(c1, t.norm) <= 0 &&
               v3_dot(c2, t.norm) <= 0;

  float rad_sq = b.rad * b.rad;

  // edge 1
  v3f p1 = point_line_closest(t.pos[0], t.pos[1], b.pos);
  v3f v1 = v3_sub(b.pos, p1);
  float dist_sq1 = v3_dot(v1, v1);
  int intersects = dist_sq1 < rad_sq;

  // edge 2
  v3f p2 = point_line_closest(t.pos[1], t.pos[2], b.pos);
  v3f v2 = v3_sub(b.pos, p2);
  float dist_sq2 = v3_dot(v2, v2);
  intersects |= dist_sq2 < rad_sq;

  // edge 3
  v3f p3 = point_line_closest(t.pos[2], t.pos[0], b.pos);
  v3f v3 = v3_sub(b.pos, p3);
  float dist_sq3 = v3_dot(v3, v3);
  intersects |= dist_sq3 < rad_sq;

  if (!inside && !intersects) return hit_miss;

  v3f norm;

  if (inside) {
    norm = v3_sub(b.pos, p0);
  } else {
    float best_sq = dist_sq1;
    norm = v1;

    if (dist_sq2 < best_sq) {
      best_sq = dist_sq2;
      norm = v2;
    }

    if (dist_sq3 < best_sq) {
      norm = v3;
    }
  }

  float len = v3_len(norm);
  v3_norm(&norm);
  float depth = b.rad - len;
  return (hit){
    .is_hit = 1,
    .norm = norm,
    .push = depth
  };
}

hit hit_c(obj *cap_a, obj *cap_b) {
  cap a = cap_a->c;
  cap b = cap_b->c;

  v3f a_A = v3_add(a.pos, v3_mul(a.norm, a.ext + a.rad));
  v3f a_B = v3_sub(a.pos, v3_mul(a.norm, a.ext + a.rad));
  v3f b_A = v3_add(b.pos, v3_mul(b.norm, b.ext + b.rad));
  v3f b_B = v3_sub(b.pos, v3_mul(b.norm, b.ext + b.rad));

  v3f v0 = v3_sub(b_A, a_A);
  v3f v1 = v3_sub(b_B, a_A);
  v3f v2 = v3_sub(b_A, a_B);
  v3f v3 = v3_sub(b_B, a_B);

  float d0 = v3_dot(v0, v0);
  float d1 = v3_dot(v1, v1);
  float d2 = v3_dot(v2, v2);
  float d3 = v3_dot(v3, v3);

  v3f best_a = (d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1) ? a_B : a_A;

  v3f best_b = point_line_closest(b_A, b_B, best_a);

  best_a = point_line_closest(a_A, a_B, best_b);

  return hit_b(&(obj){.b = ball_new(best_a, a.rad)},
               &(obj){.b = ball_new(best_b, b.rad)});
}

hit hit_bc(obj *ball_obj, obj *cap_obj) {
  ball b = ball_obj->b;
  cap c = cap_obj->c;
  v3f best_p = point_line_closest(v3_add(c.pos, v3_mul(c.norm, c.ext)),
                                  v3_sub(c.pos, v3_mul(c.norm, c.ext)),
                                  b.pos);

  return hit_b(ball_obj, &(obj){.b = ball_new(best_p, b.rad)});
}

hit hit_tc(obj *tri_obj, obj *cap_obj) {
  tri t = tri_obj->t;
  cap c = cap_obj->c;

  v3f a = v3_add(c.pos, v3_mul(c.norm, c.ext));
  v3f b = v3_sub(c.pos, v3_mul(c.norm, c.ext));

  float T = v3_dot(t.norm, v3_div(v3_sub(t.pos[0], c.pos),
                                  fabsf(v3_dot(t.norm, c.norm))));
  v3f line_plane_intersection = v3_add(c.pos, v3_mul(c.norm, T));

  v3f ref_point;

  // Determine whether line_plane_intersection is inside all triangle edges:
  v3f c0 = v3_cross(v3_sub(line_plane_intersection, t.pos[0]),
                    v3_sub(t.pos[1], t.pos[0]));
  v3f c1 = v3_cross(v3_sub(line_plane_intersection, t.pos[1]),
                    v3_sub(t.pos[2], t.pos[1]));
  v3f c2 = v3_cross(v3_sub(line_plane_intersection, t.pos[2]),
                    v3_sub(t.pos[0], t.pos[2]));
  int inside = v3_dot(c0, t.norm) <= 0 && v3_dot(c1, t.norm) <= 0 &&
               v3_dot(c2, t.norm) <= 0;

  if (inside) {
    ref_point = line_plane_intersection;
  } else {
    // Edge 1:
    v3f point1 = point_line_closest(t.pos[0], t.pos[1],
                                    line_plane_intersection);
    v3f v1 = v3_sub(line_plane_intersection, point1);
    float distsq = v3_dot(v1, v1);
    float best_dist = distsq;
    ref_point = point1;

    // Edge 2:
    v3f point2 = point_line_closest(t.pos[1], t.pos[2],
                                    line_plane_intersection);
    v3f v2 = v3_sub(line_plane_intersection, point2);
    distsq = v3_dot(v2, v2);
    if (distsq < best_dist) {
      ref_point = point2;
      best_dist = distsq;
    }

    // Edge 3:
    v3f point3 = point_line_closest(t.pos[2], t.pos[0],
                                    line_plane_intersection);
    v3f v3 = v3_sub(line_plane_intersection, point3);
    distsq = v3_dot(v3, v3);
    if (distsq < best_dist) {
      ref_point = point3;
    }
  }

  v3f center = point_line_closest(a, b, ref_point);

  return hit_tb(tri_obj, &(obj){.b = ball_new(center, c.rad)});
}

hit hit_mb(obj *tmesh_obj, obj *ball_obj) {
  tmesh m = tmesh_obj->m;
  v3f total = v3_zero;
  for (obj *o = m.tris, *end = arr_end(m.tris); o != end; o++) {
    hit h = hit_tb(o, ball_obj);

    total = v3_add(total, v3_mul(h.norm, h.push));
  }

  float push = v3_len(total);

  if (push < 0.00001) return hit_miss;

  v3f norm = v3_div(total, push);

  return (hit){
    .is_hit = 1,
    .norm = norm,
    .push = push
  };
}

hit hit_mc(obj *tmesh_obj, obj *cap_obj) {
  tmesh m = tmesh_obj->m;
  v3f total = v3_zero;
  for (obj *o = m.tris, *end = arr_end(m.tris); o != end; o++) {
    hit h = hit_tc(o, cap_obj);

    total = v3_add(total, v3_mul(h.norm, h.push));
  }

  float push = v3_len(total);

  if (push < 0.00001) return hit_miss;

  v3f norm = v3_div(total, push);

  return (hit){
    .is_hit = 1,
    .norm = norm,
    .push = push
  };
}

hit obj_hit(obj *a, obj *b) {
  if (!obj_is_quick_intersecting(a, b)) return hit_miss;

  if (a->type > b->type) {
    hit h = obj_hit(b, a);
    h.norm = v3_inv(h.norm);
    return h;
  }

  switch (a->type | b->type) {
    case ot_ball: return hit_b(a, b);
    case ot_cap: return hit_c(a, b);
    case ot_cap | ot_ball: return hit_bc(b, a);
    case ot_tri | ot_ball: return hit_tb(a, b);
    case ot_tri | ot_cap: return hit_tc(a, b);
    case ot_mesh | ot_ball: return hit_mb(a, b);
    case ot_mesh | ot_cap: return hit_mc(a, b);
    case ot_tri: throw_c("obj_hit: two triangles may not collide!");
  }
}

v3f *obj_get_pos(obj *o) {
  switch (o->type) {
    case ot_ball: return &o->b.pos;
    case ot_cap: return &o->c.pos;
    case ot_tri: throw_c("obj_get_pos: can't get pos of triangle");
  }
}

void obj_move(obj *o, v3f d) {
  switch (o->type) {
    case ot_tri: throw_c("obj_move: can't move a triangle!");
    case ot_ball: o->b.pos = v3_add(o->b.pos, d);
      break;
    case ot_cap: o->c.pos = v3_add(o->c.pos, d);
      break;
  }
}

void obj_draw(obj *o, cam *c, float d) {
  static imod *ball = NULL, *cyl = NULL;
  if (!ball) {
    ball = imod_new(mod_new("res/ball.obj"));
    cyl = imod_new(mod_new("res/cylinder.obj"));
  }

  switch (o->type) {
    case ot_ball: {
      auto b = o->b;
      float r = o->b.rad;
      imod_add(ball, m4_mul(m4_scale(r, r, r),
                               m4_trans_v(v3_lerp(b.prev_pos, b.pos, d))));
      break;
    }
    case ot_cap: {
      float r = o->c.rad;
      imod_add(cyl,
               m4_mul(m4_scale(r, o->c.ext, r),
                      m4_mul(m4_chg_axis(o->c.norm, 1),
                             m4_trans_v(v3_lerp(o->c.prev_pos, o->c.pos, d)))));
      imod_add(ball, m4_mul(m4_scale(r, r, r), m4_trans_v(
        v3_add(v3_lerp(o->c.prev_pos, o->c.pos, d),
               v3_mul(o->c.norm, o->c.ext)))));
      imod_add(ball, m4_mul(m4_scale(r, r, r), m4_trans_v(
        v3_sub(v3_lerp(o->c.prev_pos, o->c.pos, d),
               v3_mul(o->c.norm, o->c.ext)))));
      break;
    }
    default: return;
  }
}

cap cap_new(v3f pos, v3f norm, float rad, float ext) {
  return (cap){
    .type = ot_cap,
    .pos = pos,
    .rad = rad,
    .ext = ext,
    .norm = norm
  };
}

tri tri_new(v3f a, v3f b, v3f c, v3f norm, bool double_sided) {
  return (tri){
    .type = ot_tri,
    .pos = {a, b, c},
    .norm = norm,
    .is_double_sided = double_sided
  };
}

ball ball_new(v3f pos, float rad) {
  return (ball){
    .type = ot_ball,
    .pos = pos,
    .rad = rad,
    .prev_pos = pos,
    .vel = v3_zero
  };
}

void obj_tick(obj *o, float t) {
  *obj_get_pos(o) = v3_add(*obj_get_pos(o), *obj_get_vel(o));
  obj_get_vel(o)->y -= 0.0075f * t * t;
}

v3f *obj_get_prev_pos(obj *o) {
  switch (o->type) {
    case ot_ball: return &o->b.prev_pos;
    case ot_tri: throw_c("obj_get_prev_pos: triangle has no prev_pos");
    case ot_cap: return &o->c.prev_pos;
  }
}

v3f *obj_get_vel(obj *o) {
  switch (o->type) {
    case ot_ball: return &o->b.vel;
    case ot_tri: throw_c("obj_get_vel: triangle has no vel");
    case ot_cap: return &o->c.vel;
  }
}

tmesh tmesh_new(tri *tris) {
  auto objs = arr_new(obj, arr_len(tris));

  float large = 1e20f;
  v3f min = (v3f){large, large, large}, max = (v3f){-large, -large, -large};

  for (int i = 0, size = arr_len(tris); i < size; i++) {
    arr_add(&objs, &(obj){.t = tris[i]});
    for (int j = 0; j < 3; j++) {
      v3f pos = tris[i].pos[j];
      min.x = min(pos.x, min.x);
      min.y = min(pos.y, min.y);
      min.z = min(pos.z, min.z);
      max.x = max(pos.x, max.x);
      max.y = max(pos.y, max.y);
      max.z = max(pos.z, max.z);
    }
  }

  arr_del(tris);

  return (tmesh){
    .type = ot_mesh,
    .tris = objs,
    .box = (box3){.min = min, .max = max}
  };
}

bool obj_is_quick_intersecting(obj *a, obj *b) {
  return box3_is_overlap(obj_get_box(a), obj_get_box(b));
}

box3 obj_get_box(obj *o) {
  switch (o->type) {
    case ot_mesh: return o->m.box;
    case ot_ball:
      return (box3){.min = v3_add(o->b.pos,
                                  (v3f){-o->b.rad, -o->b.rad,
                                        -o->b.rad}), .max = v3_add(
        o->b.pos, (v3f){o->b.rad, o->b.rad, o->b.rad})};
    case ot_cap: {
      return box3_fit_2(obj_get_box(&(obj){.b = ball_new(v3_add(o->c.pos,
                                                                v3_mul(
                                                                  o->c.norm,
                                                                  o->c.ext)),
                                                         o->c.rad)}),
                        obj_get_box(&(obj){.b = ball_new(v3_sub(o->c.pos,
                                                                v3_mul(
                                                                  o->c.norm,
                                                                  o->c.ext)),
                                                         o->c.rad)}));
    }
    case ot_tri: throw_c("obj_get_box: not implemented yet!");
  }
}

