#include "obj.h"
#include "world.h"
#include "app.h"
#include "hana.h"

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
    "res/bush2_trunk.obj",
    "res/bush3_trunk.obj",
    "res/tree1_trunk.obj",
    "res/tree2_trunk.obj",
    "res/bush2_trunk_dec.obj",
    "res/bush3_trunk_dec.obj",
    "res/tree1_trunk_dec.obj",
    "res/tree2_trunk_dec.obj",
  };

  static char const *leaf_paths[n_trees * 2] = {
    "res/bush2_leaves.obj",
    "res/bush3_leaves.obj",
    "res/tree1_leaves.obj",
    "res/tree2_leaves.obj",
    "res/bush2_leaves_dec.obj",
    "res/bush3_leaves_dec.obj",
    "res/tree1_leaves_dec.obj",
    "res/tree2_leaves_dec.obj",
  };

  static char const *mtl_paths[n_trees] = {
    "res/bush2.obj",
    "res/bush3.obj",
    "res/tree1.obj",
    "res/tree2.obj",
  };
#else
  static char const *trunk_paths[n_trees * 2] = {
    "res/bush2_trunk.obj",
    "res/bush2_trunk_dec.obj",
  };

  static char const *leaf_paths[n_trees * 2] = {
    "res/bush2_leaves.obj",
    "res/bush2_leaves_dec.obj",
  };

  static char const *mtl_paths[n_trees] = {
    "res/bush2.obj",
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
  lazy.hana = objdup(mod_new_mem(hana_str, hana_str_len, "res/hana.obj"));
#endif

  lazy.ball = imod_new(mod_new("res/ball.obj"));
  lazy.cyl = imod_new(mod_new("res/cylinder.obj"));

  lazy.init = 1;
}

void hana_draw(obj *o, draw_src s, cam *c, float d) {
  cap cap = o->body.cap;
  v3f base = v3_sub(obj_get_ipos(o, d), v3_mul(cap.norm, cap.ext + cap.rad));
  mod_draw(lazy.hana, s, c, m4_mul(m4_trans(0, 0, 0.215f),
                                   m4_mul(m4_rot_y(
                                            -rad(the_app.cam.yaw) + M_PIF / 2.f),
                                          m4_trans_v(base))));
}

void obj_draw(obj *o, draw_src s, cam *c, float d) {
  lazy_init();

  switch (o->type) {
    case ot_hana: {
#ifdef NDEBUG
      hana_draw(o, s, c, d);
#else
      float r = o->body.cap.rad, ext = o->body.cap.ext;
      v3f norm = o->body.cap.norm;
      imod_add(lazy.cyl, m4_mul(m4_scale(r, ext, r), m4_trans_v(obj_get_ipos(o, d))));
      imod_add(lazy.ball, m4_mul(m4_scale(r, r, r), m4_trans_v(
        v3_add(obj_get_ipos(o, d), v3_mul(norm, ext)))));
      imod_add(lazy.ball, m4_mul(m4_scale(r, r, r), m4_trans_v(
        v3_add(obj_get_ipos(o, d), v3_neg(v3_mul(norm, ext))))));
#endif
      break;
    }
    case ot_test: {
      float r = o->body.ball.rad;
      imod_add(lazy.ball,
               m4_mul(m4_scale(r, r, r), m4_trans_v(obj_get_ipos(o, d))));
      break;
    }
    case ot_tree: {
      tree *t = &o->tree;
      int lod = n_trees *
                (box3_dist(t->box, cam_get_eye(c)) >
                 36.f);

      imod_add(lazy.leaves[t->idx + lod],
               m4_mul(m4_mul(m4_rot_y(t->rot), m4_chg_axis(t->dir, 1)),
                      m4_trans_v(v3_sub(o->body.pos, t->offset))));

      imod_add(lazy.trunks[t->idx + lod],
               m4_mul(m4_mul(m4_rot_y(t->rot), m4_chg_axis(t->dir, 1)),
                      m4_trans_v(v3_sub(o->body.pos, t->offset))));
      break;
    }
  }
}

void hana_tick(obj *o) {
  app *a = &the_app;
  body *b = &o->body;

  float forwards = 0, sideways = 0;
  float up = app_is_key_down(a, GLFW_KEY_SPACE) && b->on_ground;
  if (app_is_key_down(a, GLFW_KEY_W)) forwards++;
  if (app_is_key_down(a, GLFW_KEY_A)) sideways--;
  if (app_is_key_down(a, GLFW_KEY_S)) forwards--;
  if (app_is_key_down(a, GLFW_KEY_D)) sideways++;

  v3f front_xz = a->cam.front;
  front_xz.y = 0.f;
  if (v3_len(front_xz) >= 0.0001) {
    v3_norm(&front_xz);
  }

  v3f final = v3_add(v3_mul(front_xz, forwards),
                     v3_mul(a->cam.right, sideways));
  bool moving = v3_dot(final, final) > 0.0001;
  if (moving) v3_norm(&final);
  final = v3_add(final, v3_mul(v3_mul(a->cam.world_up, up), 3.f));
  final = v3_mul(final, 0.015f);

  if (!b->on_ground) final = v3_mul(final, 0.05f);

  b->vel = v3_mul_v(v3_add(b->vel, final),
                    b->on_ground ? v3_one : (v3f){0.975f, 1.f, 0.975f});

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

v3f obj_get_ipos(obj *o, float d) {
  return body_get_ipos(&o->body, d);
}

obj test_new(v3f pos, v3f vel, float rad) {
  return (obj){
    .type = ot_test,
    .body = {
      .ball = ball_new(rad),
      .pos = pos,
      .vel = vel,
      .slip = 0.99f},
    .dynamic = 1};
}

obj tree_new(v3f pos, v3f dir) {
  lazy_init();

  static tmesh meshes[n_trees];
  static cap phys[n_trees];
  static v3f off[n_trees];
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
  switch (o->type) {
    case ot_hana: {
      cap c = o->body.cap;
      return box3_add(lazy.hana->bounds,
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
