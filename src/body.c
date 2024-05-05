#include "body.h"
#include "typedefs.h"
#include "err.h"
#include "arr.h"
#include "box.h"
#include <stddef.h>
#include "gl.h"

void body_response(body *b, hit h, float slip) {
  b->pos = v3_add(b->pos, v3_mul(h.norm, h.push));
  float vel_len = v3_len(b->vel);
  v3f vel_normed = v3_div(b->vel, vel_len);
  v3f undesired_motion = v3_mul(h.norm, v3_dot(vel_normed, h.norm));
  v3f desired_motion = v3_sub(vel_normed, undesired_motion);
  b->vel = v3_mul(desired_motion, vel_len * slip);
  b->on_ground = 1;
}

hit hit_b(body *a, body *b) {
  float len = a->ball.rad + b->ball.rad;
  v3f a_pos = a->pos, b_pos = b->pos;
  float dist = v3_dist(a_pos, b_pos);
  if (dist > len) {
    return hit_miss;
  }

  float push = len - dist;
  v3f norm = v3_normed(v3_sub(b_pos, a_pos));

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

hit hit_tb(tri t, body *ball_obj) {
  v3f A = t.pos[0], B = t.pos[1], C = t.pos[2];
  ball b = ball_obj->ball;
  v3f b_pos = ball_obj->pos;
  float dist = v3_dot(v3_sub(b_pos, A), t.norm);
  if (dist < -b.rad || dist > b.rad) return hit_miss;

  v3f p0 = v3_sub(b_pos, v3_mul(t.norm, dist));
  v3f c0 = v3_cross(v3_sub(p0, A), v3_sub(B, A));
  v3f c1 = v3_cross(v3_sub(p0, B), v3_sub(C, B));
  v3f c2 = v3_cross(v3_sub(p0, C), v3_sub(A, C));

  int inside = v3_dot(c0, t.norm) <= 0 && v3_dot(c1, t.norm) <= 0 &&
               v3_dot(c2, t.norm) <= 0;

  float rad_sq = b.rad * b.rad;

  // edge 1
  v3f p1 = point_line_closest(A, B, b_pos);
  v3f v1 = v3_sub(b_pos, p1);
  float dist_sq1 = v3_dot(v1, v1);
  int intersects = dist_sq1 < rad_sq;

  // edge 2
  v3f p2 = point_line_closest(B, C, b_pos);
  v3f v2 = v3_sub(b_pos, p2);
  float dist_sq2 = v3_dot(v2, v2);
  intersects |= dist_sq2 < rad_sq;

  // edge 3
  v3f p3 = point_line_closest(C, A, b_pos);
  v3f v3 = v3_sub(b_pos, p3);
  float dist_sq3 = v3_dot(v3, v3);
  intersects |= dist_sq3 < rad_sq;

  if (!inside && !intersects) return hit_miss;

  v3f norm;

  if (inside) {
    norm = v3_sub(b_pos, p0);
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

hit hit_c(body *cap_a, body *cap_b) {
  cap a = cap_a->cap;
  cap b = cap_b->cap;
  v3f a_pos = cap_a->pos, b_pos = cap_b->pos;

  v3f a_A = v3_add(a_pos, v3_mul(a.norm, a.ext + a.rad));
  v3f a_B = v3_sub(a_pos, v3_mul(a.norm, a.ext + a.rad));
  v3f b_A = v3_add(b_pos, v3_mul(b.norm, b.ext + b.rad));
  v3f b_B = v3_sub(b_pos, v3_mul(b.norm, b.ext + b.rad));

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

  return hit_b(&(body){.ball = ball_new(a.rad), .pos = best_a},
               &(body){.ball = ball_new(b.rad), .pos = best_b});
}

hit hit_bc(body *ball_obj, body *cap_obj) {
  ball b = ball_obj->ball;
  cap c = cap_obj->cap;
  v3f c_pos = cap_obj->pos, b_pos = ball_obj->pos;

  v3f best_p = point_line_closest(v3_add(c_pos, v3_mul(c.norm, c.ext)),
                                  v3_sub(c_pos, v3_mul(c.norm, c.ext)),
                                  b_pos);

  return hit_b(&(body){.ball = ball_new(b.rad), .pos = best_p}, ball_obj);
}

hit hit_tc(tri t, body *cap_obj) {
  v3f A = t.pos[0], B = t.pos[1], C = t.pos[2];
  cap c = cap_obj->cap;
  v3f c_pos = cap_obj->pos;

  v3f a = v3_add(c_pos, v3_mul(c.norm, c.ext));
  v3f b = v3_sub(c_pos, v3_mul(c.norm, c.ext));

  float T = v3_dot(t.norm, v3_div(v3_sub(A, c_pos),
                                  fabsf(v3_dot(t.norm, c.norm))));
  v3f line_plane_intersection = v3_add(c_pos, v3_mul(c.norm, T));

  v3f ref_point;

  // Determine whether line_plane_intersection is inside all triangle edges:
  v3f c0 = v3_cross(v3_sub(line_plane_intersection, A),
                    v3_sub(B, A));
  v3f c1 = v3_cross(v3_sub(line_plane_intersection, B),
                    v3_sub(C, B));
  v3f c2 = v3_cross(v3_sub(line_plane_intersection, C),
                    v3_sub(A, C));
  int inside = v3_dot(c0, t.norm) <= 0 && v3_dot(c1, t.norm) <= 0 &&
               v3_dot(c2, t.norm) <= 0;

  if (inside) {
    ref_point = line_plane_intersection;
  } else {
    // Edge 1:
    v3f point1 = point_line_closest(A, B,
                                    line_plane_intersection);
    v3f v1 = v3_sub(line_plane_intersection, point1);
    float distsq = v3_dot(v1, v1);
    float best_dist = distsq;
    ref_point = point1;

    // Edge 2:
    v3f point2 = point_line_closest(B, C,
                                    line_plane_intersection);
    v3f v2 = v3_sub(line_plane_intersection, point2);
    distsq = v3_dot(v2, v2);
    if (distsq < best_dist) {
      ref_point = point2;
      best_dist = distsq;
    }

    // Edge 3:
    v3f point3 = point_line_closest(C, A,
                                    line_plane_intersection);
    v3f v3 = v3_sub(line_plane_intersection, point3);
    distsq = v3_dot(v3, v3);
    if (distsq < best_dist) {
      ref_point = point3;
    }
  }

  v3f center = point_line_closest(a, b, ref_point);

  return hit_tb(t, &(body){.ball = ball_new(c.rad), .pos = center});
}

hit hit_mb(body *tmesh_obj, body *ball_obj) {
  tmesh m = tmesh_obj->mesh;
  v3f total = v3_zero;
  for (tri *b = m.tris, *end = arr_end(m.tris); b != end; b++) {
    if (!box3_overlaps(b->box, body_get_box(ball_obj))) continue;
    hit h = hit_tb(*b, ball_obj);

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

hit hit_mc(body *tmesh_obj, body *cap_obj) {
  tmesh m = tmesh_obj->mesh;
  v3f total = v3_zero;
  for (tri *b = m.tris, *end = arr_end(m.tris); b != end; b++) {
    if (!box3_overlaps(b->box, body_get_box(cap_obj))) continue;
    hit h = hit_tc(*b, cap_obj);

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

hit body_hit(body *a, body *b) {
  if (!body_is_hit_plausible(a, b)) return hit_miss;

  if (a->type > b->type) {
    return hit_inv(body_hit(b, a));
  }

  switch (a->type | b->type) {
    case bt_ball: return hit_b(a, b);
    case bt_cap: return hit_c(a, b);
    case bt_cap | bt_ball: return hit_bc(b, a);
    case bt_mesh | bt_ball: return hit_mb(a, b);
    case bt_mesh | bt_cap: return hit_mc(a, b);
    case bt_mesh: throw_c("body_hit: two meshes may not collide!");
  }
}

cap cap_new(v3f norm, float rad, float ext) {
  return (cap){
    .type = bt_cap,
    .rad = rad,
    .ext = ext,
    .norm = norm
  };
}

tri tri_new(v3f a, v3f b, v3f c, v3f norm) {
  return (tri){
    .pos = {a, b, c},
    .norm = norm,
    .box = box3_new(v3_min(v3_min(a, b), c), v3_max(v3_max(a, b), c))
  };
}

ball ball_new(float rad) {
  return (ball){
    .type = bt_ball,
    .rad = rad,
  };
}

void body_tick(body *b, float t) {
  b->pos = v3_add(b->pos, b->vel);
  b->vel.y -= 0.0981f * t * t * 0.33f * 0.33f;
}

tmesh tmesh_new(tri *tris) {
  auto objs = arr_new(tri, arr_len(tris));

  float large = 1e20f;
  v3f min = (v3f){large, large, large}, max = (v3f){-large, -large, -large};

  for (int i = 0, size = arr_len(tris); i < size; i++) {
    arr_add(&objs, &tris[i]);
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
    .type = bt_mesh,
    .tris = objs,
    .box = (box3){.min = min, .max = max}
  };
}

box3 body_get_box(body *b) {
  switch (b->type) {
    case bt_mesh: return b->mesh.box;
    case bt_ball: {
      ball ball = b->ball;
      return (box3){.min = v3_add(b->pos,
                                  (v3f){-ball.rad, -ball.rad,
                                        -ball.rad}), .max = v3_add(
        b->pos, (v3f){ball.rad, ball.rad, ball.rad})};
    }
    case bt_cap: {
      return box3_fit(
        body_get_box(&(body){.ball = ball_new(b->cap.rad), .pos = v3_add(b->pos,
                                                                         v3_mul(
                                                                           b->cap.norm,
                                                                           b->cap.ext))}),
        body_get_box(&(body){.ball = ball_new(b->cap.rad), .pos = v3_sub(b->pos,
                                                                         v3_mul(
                                                                           b->cap.norm,
                                                                           b->cap.ext))}));
    }
  }
}

tmesh tmesh_new_vi(obj_vtx *verts, int *inds) {
  size_t count = arr_len(inds);
  if (count % 3 != 0) throw_c("tmesh_new_vi: inds passed not for triangles!");

  tri *mesh = arr_new(tri, count / 3);
  float const big = 1000000000.f;
  v3f min = {big, big, big}, max = {-big, -big, -big};
  for (int i = 0; i < count; i += 3) {
    v3f a = verts[inds[i]].pos, b = verts[inds[i + 1]].pos, c = verts[inds[i +
                                                                           2]].pos;
    tri t = tri_new(a, b, c, v3_normed(v3_cross(v3_sub(b, a), v3_sub(c, a))));
    arr_add(&mesh, &t);
    min = v3_min(v3_min(v3_min(min, a), b), c);
    max = v3_max(v3_max(v3_max(max, a), b), c);
  }

  return (tmesh){.tris = mesh, .box = box3_new(min, max), .type = bt_mesh};
}

v3f body_get_ipos(body *o, float d) {
  return v3_lerp(o->prev_pos, o->pos, d);
}

hit hit_inv(hit h) {
  return (hit){
    h.is_hit,
    v3_inv(h.norm),
    h.push,
  };
}

tmesh tmesh_add(tmesh *orig, v3f pos) {
  tmesh m = {
    .type = bt_mesh,
    .box = box3_new(v3_add(orig->box.min, pos), v3_add(orig->box.max, pos)),
    .tris = arr_new(tri, 4)
  };

  arr_copy(&m.tris, orig->tris);

  for (tri *t = m.tris, *end = arr_end(m.tris); t != end; t++) {
    v3_inc(&t->pos[0], pos);
    v3_inc(&t->pos[1], pos);
    v3_inc(&t->pos[2], pos);
  }

  return m;
}

