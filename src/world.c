#include "world.h"
#include "typedefs.h"
#include "body.h"
#include "lmap.h"

world *world_new(obj player) {
  auto w = objdup((world){
    .chunks = lmap_new(16, sizeof(v2i), sizeof(chunk), 0.5f, iv2_eq, iv2_hash),
    .objs = arr_new(obj, 4),
    .objs_tick = arr_new(obj, 4),
    .objs_to_add = arr_new(obj, 4),
    .draw_lock = PTHREAD_MUTEX_INITIALIZER
  });

  world_add_obj(w, &player);

  for (int i = -world_draw_dist; i <= world_draw_dist; i++) {
    for (int j = -world_draw_dist; j <= world_draw_dist; j++) {
      chunk c = chunk_new((v2i){i, j});
      lmap_add(&w->chunks, &(v2i){i, j}, &c);
    }
  }

  for (int i = 0; i < world_sp_size; i++) {
    for (int j = 0; j < world_sp_size; j++) {
      w->regions[i][j] = reg_new();
    }
  }

  arr_add_bulk(&w->objs_tick, w->objs_to_add);
  arr_clear(w->objs_to_add);
  arr_copy(&w->objs, w->objs_tick);

  return w;
}

v2i world_get_chunk_pos(v3f world_pos) {
  return (v2i){(int)(world_pos.x / (float)chunk_size),
               (int)(world_pos.z / (float)chunk_size)};
}

void world_tick(world *w, cam *c) {
  // clear the reg partition
  for (int i = 0; i < world_sp_size; i++) {
    for (int j = 0; j < world_sp_size; j++) {
      reg *s = &w->regions[i][j];
      reg_clear(s);
    }
  }

  // handle all chunks
  v2i cam_pos = world_get_chunk_pos(c->pos);
  for (int i = -world_draw_dist; i <= world_draw_dist; i++) {
    for (int j = -world_draw_dist; j <= world_draw_dist; j++) {
      if (sqrt(i * i + j * j) > world_draw_dist + 1) continue;

      v2i chunk_pos = {cam_pos.x + i, cam_pos.y + j};

      chunk *ch = lmap_at(&w->chunks, &chunk_pos);
      if (!ch) continue;

      reg *r =
        &w->regions[i + world_draw_dist][j + world_draw_dist];

      reg_add_sta(r, &ch->body);
    }
  }

  // handle all objects
  for (obj *o = w->objs_tick, *end = arr_end(w->objs_tick); o != end; o++) {
    body *b = &o->body;
    b->prev_pos = b->pos;
    v2i body_pos = world_get_chunk_pos(b->pos);
    v2i off = iv2_sub(body_pos, cam_pos);
    for (int i = -1; i <= 1; i++) {
      for (int j = -1; j <= 1; j++) {
        if (abs(off.x + i) <= world_draw_dist &&
            abs(off.y + j) <= world_draw_dist) {
          reg *r = &w->regions[off.x + i + world_draw_dist][off.y + j +
                                                            world_draw_dist];
          reg_add_dyn(r, b);
          b->on_ground = 0, b->hits = 0, b->total_slip = 0;
        }
      }
    }
  }

  // tick each physics region
  int const sub_steps = 4;
  float const step_time = 1.f / (float)sub_steps;

  for (int t = 0; t < sub_steps; t++) {
    for (obj *o = w->objs_tick, *end = arr_end(w->objs_tick); o != end; o++) {
      if (v3_dist(o->body.pos, c->pos) > (world_draw_dist + 1) * chunk_size)
        continue;

      body_tick(&o->body, step_time);
    }

    for (int i = 0; i < world_sp_size; i++) {
      for (int j = 0; j < world_sp_size; j++) {
        reg *s = &w->regions[i][j];
        if (!reg_is_tickable(s)) continue;

        reg_tick(s);
      }
    }
  }

  // tick all game objects
  for (obj *o = w->objs_tick, *end = arr_end(w->objs_tick); o != end; o++) {
    body *b = &o->body;
    if (b->hits) b->vel = v3_mul(b->vel, b->total_slip / b->hits);
    if (v3_dist(b->pos, c->pos) > world_draw_dist * chunk_size) {
      continue;
    }

    obj_tick(o);
  }
}

void world_draw(world *w, cam *c, float d) {
  v2i cam_to_chunk = world_get_chunk_pos(c->pos);

  static mtl chunk_mtl = {
    .light = 6,
    .dark = 0,
    .light_model = {0, 0.8f, 0}
  };

  mod_get_sh(c, chunk_mtl, m4_ident);

  int n_gen = 0;
  for (int i = -world_draw_dist; i <= world_draw_dist; i++) {
    for (int j = -world_draw_dist; j <= world_draw_dist; j++) {
      float dist = sqrtf(i * i + j * j);
      if (dist > world_draw_dist + 1) continue;
      if (rad_wrap(atan2(i, j) + rad(c->yaw) + M_PIF * 0.125f) <= rad(c->zoom) * 1.33f && dist > 6) continue;

      v2i chunk_pos = {cam_to_chunk.x + i, cam_to_chunk.y + j};

      if (!lmap_has(&w->chunks, &chunk_pos) && n_gen < 16) {
        chunk gen = chunk_new(chunk_pos);
        lmap_add(&w->chunks, &chunk_pos, &gen);
        n_gen++;
      }

      chunk *ch = lmap_at(&w->chunks, &chunk_pos);
      if (!ch) continue;

      vao_bind(&ch->vao);
      gl_draw_elements(GL_TRIANGLES, ch->n_inds, GL_UNSIGNED_INT, 0);
    }
  }

  for (obj *o = w->objs, *end = arr_end(w->objs); o != end; o++) {
    body *b = &o->body;
    if (v3_dist(b->pos, c->pos) > world_draw_dist * chunk_size) {
      continue;
    }

    obj_draw(o, c, d);
  }
}

void world_add_obj(world *w, obj *o) {
  o->body.prev_pos = o->body.pos;
  o->world = w;
  o->id = arr_len(w->objs_tick);
  arr_add(&w->objs_to_add, o);
}