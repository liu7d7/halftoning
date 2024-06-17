#include "obj.h"
#include "world.h"
#include "app.h"

#ifdef NDEBUG
#define n_trees 4
#else
#define n_trees 1
#endif

static struct {
  mod *hana;
  imod *ball, *cyl, *trunks[n_trees * 2], *leaves[n_trees * 2];
  int init;
} lazy;

void lazy_init() {
  if (lazy.init) return;

#ifdef NDEBUG
  static char const *trunk_paths[n_trees * 2] = {
    "res/bush2_trunk.glb",
    "res/bush3_trunk.glb",
    "res/tree1_trunk.glb",
    "res/tree2_trunk.glb",
    "res/bush2_trunk_dec.glb",
    "res/bush3_trunk_dec.glb",
    "res/tree1_trunk_dec.glb",
    "res/tree2_trunk_dec.glb",
  };

  static char const *leaf_paths[n_trees * 2] = {
    "res/bush2_leaves.glb",
    "res/bush3_leaves.glb",
    "res/tree1_leaves.glb",
    "res/tree2_leaves.glb",
    "res/bush2_leaves_dec.glb",
    "res/bush3_leaves_dec.glb",
    "res/tree1_leaves_dec.glb",
    "res/tree2_leaves_dec.glb",
  };

  static char const *mtl_paths[n_trees] = {
    "res/bush2.glb",
    "res/bush3.glb",
    "res/tree1.glb",
    "res/tree2.glb",
  };
#else
  static char const *trunk_paths[n_trees * 2] = {
    "res/bush2_trunk.glb",
    "res/bush2_trunk_dec.glb",
  };

  static char const *leaf_paths[n_trees * 2] = {
    "res/bush2_leaves.glb",
    "res/bush2_leaves_dec.glb",
  };

  static char const *mtl_paths[n_trees] = {
    "res/bush2.glb",
  };
#endif

  for (int i = 0; i < n_trees; i++) {
    lazy.leaves[i] = imod_new(
      mod_new_indirect_mtl(leaf_paths[i], mtl_paths[i]));
    lazy.leaves[i + n_trees] = imod_new(
      mod_new_indirect_mtl(leaf_paths[i + n_trees], mtl_paths[i]));
    lazy.trunks[i] = imod_new(
      mod_new_indirect_mtl(trunk_paths[i], mtl_paths[i]));
    lazy.trunks[i + n_trees] = imod_new(
      mod_new_indirect_mtl(trunk_paths[i + n_trees], mtl_paths[i]));
  }

#ifdef NDEBUG
  lazy.hana = _new_(mod_new("res/hana.glb"));
#endif

  lazy.ball = imod_new(mod_new("res/ball.glb"));
  lazy.cyl = imod_new(mod_new("res/cylinder.glb"));

  lazy.init = 1;
}

void hana_draw(obj *o, draw_src s, cam *c, float d) {
  cap cap = o->body.cap;
  v3 base = v3_sub(obj_get_ipos(o, d), v3_mul(cap.norm, cap.ext + cap.rad));
  mod_draw(lazy.hana, s, c, m4_mul(m4_trans(0, 0, 0.215f),
                                   m4_mul(m4_rot_y(
                                            -rad($.cam.yaw) + M_PIF / 2.f),
                                          m4_trans_v(base))), o->id);
}

void obj_draw(obj *o, draw_src s, cam *c, float d) {
  lazy_init();

  switch (o->type) {
    case ot_hana: {
#ifdef NDEBUG
      hana_draw(o, s, c, d);
#else
      float r = o->body.cap.rad, ext = o->body.cap.ext;
      v3 norm = o->body.cap.norm;
      imod_add(lazy.cyl, m4_mul(m4_scale(r, ext, r), m4_trans_v(obj_get_ipos(o, d))), o->id);
      imod_add(lazy.ball, m4_mul(m4_scale(r, r, r), m4_trans_v(
        v3_add(obj_get_ipos(o, d), v3_mul(norm, ext)))), o->id);
      imod_add(lazy.ball, m4_mul(m4_scale(r, r, r), m4_trans_v(
        v3_add(obj_get_ipos(o, d), v3_neg(v3_mul(norm, ext))))), o->id);
#endif
      break;
    }
    case ot_test: {
      float r = o->body.ball.rad;
      imod_add(lazy.ball,
               m4_mul(m4_scale(r, r, r), m4_trans_v(obj_get_ipos(o, d))), o->id);
      break;
    }
    case ot_tree: {
      tree *t = &o->tree;
      int lod = n_trees *
                (box3_dist(t->box, cam_get_eye(c)) >
                 36.f);

      imod_add(lazy.leaves[t->idx + lod],
               m4_mul(m4_mul(m4_rot_y(t->rot), m4_chg_axis(t->dir, 1)),
                      m4_trans_v(v3_sub(o->body.pos, t->offset))), o->id);

      imod_add(lazy.trunks[t->idx + lod],
               m4_mul(m4_mul(m4_rot_y(t->rot), m4_chg_axis(t->dir, 1)),
                      m4_trans_v(v3_sub(o->body.pos, t->offset))), o->id);
      break;
    }
  }
}

void hana_tick(obj *o) {
  app *a = &$;
  body *b = &o->body;

  float forwards = 0, sideways = 0;
  float up = app_is_key_down(a, GLFW_KEY_SPACE) && b->on_ground;
  if (app_is_key_down(a, GLFW_KEY_W)) forwards++;
  if (app_is_key_down(a, GLFW_KEY_A)) sideways--;
  if (app_is_key_down(a, GLFW_KEY_S)) forwards--;
  if (app_is_key_down(a, GLFW_KEY_D)) sideways++;

  v3 front_xz = a->cam.front;
  front_xz.y = 0.f;
  if (v3_len(front_xz) >= 0.0001) {
    v3_norm(&front_xz);
  }

  v3 final = v3_add(v3_mul(front_xz, forwards),
                     v3_mul(a->cam.right, sideways));
  bool moving = v3_dot(final, final) > 0.0001;
  if (moving) v3_norm(&final);
  final = v3_add(final, v3_mul(v3_mul(a->cam.world_up, up), 3.f));
  final = v3_mul(final, 0.015f);

  if (!b->on_ground) final = v3_mul(final, 0.05f);

  b->vel = v3_mul_v(v3_add(b->vel, final),
                    b->on_ground ? v3_one : (v3){0.975f, 1.f, 0.975f});

  if (app_is_key_down(a, GLFW_KEY_T)) {
    cap c = o->body.cap;
    obj t = test_new(v3_add(v3_add(o->body.pos, v3_mul(c.norm, c.ext + c.rad)),
                            v3_mul(a->cam.front, 3)),
                     v3_mul(a->cam.front, 0.1f), 0.5f);
    world_add_obj(o->world, &t);
  }
}

void obj_tick(obj *o) {
  switch (o->type) {
    case ot_hana: hana_tick(o);
      break;
    default:;
  }
}

obj hana_new() {
  return (obj){
    .hana = {.type = ot_hana},
    .body = {
      .cap = cap_new(v3_uy,
                     0.4f,
                     1.83f),
      .pos = {0, 30.f, 0},
      .slip = 0.85f},
    .dynamic = 1};
}

v3 obj_get_ipos(obj *o, float d) {
  return body_get_ipos(&o->body, d);
}

obj test_new(v3 pos, v3 vel, float rad) {
  return (obj){
    .type = ot_test,
    .body = {
      .ball = ball_new(rad),
      .pos = pos,
      .vel = vel,
      .slip = 0.99f},
    .dynamic = 1};
}

obj tree_new(v3 pos, v3 dir) {
  lazy_init();

  static tmesh meshes[n_trees];
  static cap phys[n_trees];
  static v3 off[n_trees];
  static int first_run = 1;
  if (first_run) {
    phys[0] = cap_new(v3_uy, 1.2f, 0.75f);
#ifdef NDEBUG
    phys[1] = cap_new(v3_uy, 1.f, 1.65f);
    phys[2] = cap_new(v3_uy, 0.346f, 3.62f);
    off[2] = v3_mul(v3_uy, 3.62f);
    phys[3] = cap_new(v3_uy, 0.346f, 3.62f);
    off[3] = v3_mul(v3_uy, 3.62f);
#endif

    first_run = 0;
  }

  int idx = rndi(0, n_trees);
  if (idx >= 2) dir = v3_uy;

  return (obj){
    .tree = {
      .type = ot_tree,
      .idx = idx,
      .offset = v3_add(off[idx], v3_mul(dir, 0.25f)),
      .dir = dir,
      .rot = rndf(0, 2.f * M_PIF),
      .box = box3_add(
        box3_fit(lazy.trunks[idx]->bounds, lazy.leaves[idx]->bounds),
        v3_sub(pos, off[idx]))},
    .dynamic = 0,
    .body = {
      .cap = phys[idx],
      .pos = v3_add(pos, off[idx]),
      .slip = 0.9f}
  };
}

box3 obj_get_box(obj *o) {
#ifndef NDEBUG
  lazy_init();
#endif
  switch (o->type) {
    case ot_hana: {
      cap c = o->body.cap;
      return box3_add(
#ifdef NDEBUG
        lazy.hana->bounds,
#else
        lazy.cyl->bounds,
#endif
                      v3_sub(o->body.pos, v3_mul(c.norm, c.ext + c.rad)));
    }
    case ot_tree: {
      return o->tree.box;
    }
    case ot_test: {
      return body_get_box(&o->body);
    }
  }
}

float obj_raycast(obj *e, v3 o, v3 d) {
  switch (e->type) {
    case ot_hana:
    case ot_tree:
    case ot_test:
      
  }
}
