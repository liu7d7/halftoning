#include "obj.h"
#include "world.h"
#include "app.h"
#include "hana.h"

void hana_draw(obj *o, cam *c, float d) {
  static mod *m = NULL;
  if (!m) {
    m = objdup(mod_new_mem(hana_str, hana_str_len, "res/hana.obj"));
  }

  cap cap = o->body.cap;
  v3f base = v3_sub(obj_get_ipos(o, d), v3_mul(cap.norm, cap.ext + cap.rad));
  mod_draw(m, c, m4_mul(m4_trans(0, 0, 0.215f),
                        m4_mul(m4_rot_y(-rad(c->yaw) + M_PIF / 2.f),
                               m4_trans_v(base))));
}

void obj_draw(obj *o, cam *c, float d) {
  switch (o->type) {
    case ot_hana: hana_draw(o, c, d);
      break;
    case ot_test: {
      static imod *ball = NULL;
      if (!ball) {
        ball = imod_new(mod_new("res/ball.obj"));
      }

      float r = o->body.ball.rad;
      imod_add(ball, m4_mul(m4_scale(r, r, r), m4_trans_v(obj_get_ipos(o, d))));
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

  v3f final = v3_add(v3_mul(a->cam.front, forwards),
                     v3_mul(a->cam.right, sideways));
  bool moving = v3_dot(final, final) > 0.0001;
  if (moving) v3_norm(&final);
  final = v3_add(final, v3_mul(v3_mul(a->cam.world_up, up), 3.f));
  final = v3_mul(final, 0.06f);

  if (!b->on_ground) final = v3_mul(final, 0.05f);

  b->vel = v3_mul_v(v3_add(b->vel, final),
                b->on_ground ? v3_one : (v3f){0.975f, 1.f, 0.975f});

  if (app_is_key_down(a, GLFW_KEY_T)) {
    cap c = o->body.cap;
    obj t = test_new(v3_add(v3_add(o->body.pos, v3_mul(c.norm, c.ext + c.rad)),
                            v3_mul(a->cam.front, 3)),
                     v3_mul(a->cam.front, 0.3f), 0.5f);
    world_add_obj(o->world, &t);
  }
}

void obj_tick(obj *o) {
  switch (o->type) {
    case ot_hana: hana_tick(o); break;
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
      .slip = 0.8f}};
}

v3f obj_get_ipos(obj *o, float d) {
  return body_get_ipos(&o->body, d);
}

obj test_new(v3f pos, v3f vel, float rad) {
  return (obj){.type = ot_test, .body = (body){.ball = ball_new(rad), .pos = pos, .vel = vel, .slip = 0.95f}};
}
