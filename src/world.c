#include "world.h"
#include "typedefs.h"
#include "body.h"
#include "lmap.h"
#include "app.h"

world *world_new(obj player) {
  buf vb = buf_new(GL_ARRAY_BUFFER), ib = buf_new(GL_ELEMENT_ARRAY_BUFFER);

  auto w = objdup((world){
    .chunks = lmap_new(16, sizeof(v2i), sizeof(chunk), 0.5f, iv2_peq, iv2_hash),
    .objs = arr_new(obj, 4),
    .objs_tick = arr_new(obj, 4),
    .objs_to_add = arr_new(obj, 4),
    .draw_lock = PTHREAD_MUTEX_INITIALIZER,
    .vb = vb,
    .ib = ib,
    .va = vao_new(&vb, &ib, 2, (attrib[]){attr_3f, attr_3f}),
    .vb_dirty = 0,
    .last_chunk_pos = (v2i){INT_MAX, INT_MAX},
    .vb_cache = arr_new(obj_vtx, 4),
    .ib_cache = arr_new(int, 4),
  });

  world_add_obj(w, &player);

#ifdef NDEBUG
  for (int i = -world_draw_dist; i <= world_draw_dist; i++) {
    for (int j = -world_draw_dist; j <= world_draw_dist; j++) {
      chunk c = chunk_new(w, (v2i){i, j});
      lmap_add(&w->chunks, &(v2i){i, j}, &c);
    }
  }
#endif

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
  return (v2i){(int)floorf(world_pos.x / (float)chunk_size),
               (int)floorf(world_pos.z / (float)chunk_size)};
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

      reg *r = &w->regions[i + world_draw_dist][j + world_draw_dist];

      reg_add_sta(r, &ch->body);
    }
  }

  // handle all objects
  for (obj *o = w->objs_tick, *end = arr_end(w->objs_tick); o != end; o++) {
    body *b = &o->body;
    b->prev_pos = b->pos;
    b->on_ground = 0;
    box3 bounds = body_get_box(b);
    v2i chunk_min = world_get_chunk_pos(bounds.min),
      chunk_max = world_get_chunk_pos(bounds.max);
    v2i min = iv2_sub(chunk_min, cam_pos),
      max = iv2_sub(chunk_max, cam_pos);

    for (int i = min.x; i <= max.x; i++) {
      for (int j = min.y; j <= max.y; j++) {
        if (abs(i) <= world_draw_dist &&
            abs(j) <= world_draw_dist) {
          reg *r = &w->regions[i + world_draw_dist][j + world_draw_dist];
          (o->dynamic ? reg_add_dyn : reg_add_sta)(r, b);
        }
      }
    }
  }

  // tick each physics region
  int const sub_steps = 4;
  float const step_time = 1.f / (float)sub_steps;

  for (int t = 0; t < sub_steps; t++) {
    for (obj *o = w->objs_tick, *end = arr_end(w->objs_tick); o != end; o++) {
      if (!o->dynamic || v3_dist(o->body.pos, c->pos) > (world_draw_dist + 1) * chunk_size)
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
    if (v3_dist(b->pos, c->pos) > world_draw_dist * chunk_size) {
      continue;
    }

    obj_tick(o);
  }
}

void world_cache(world *w, v2i cam_to_chunk) {
#define n_inds (chunk_qty * chunk_qty* 6)
  static int qinds[n_inds], first_run = 1;
  if (first_run) {
    int *rinds = quad_indices(chunk_len, chunk_len);
    memcpy(qinds, rinds, sizeof(qinds));
    first_run = 0;
  }

  static int last_nc = 0;

  if (!iv2_eq(w->last_chunk_pos, cam_to_chunk)) {
    arr_clear(w->vb_cache);
    int nc = 0;

    for (int i = -world_draw_dist; i <= world_draw_dist; i++) {
      for (int j = -world_draw_dist; j <= world_draw_dist; j++) {
        float dist = sqrtf(i * i + j * j);
        if (dist > world_draw_dist + 1) continue;
        nc++;

        v2i chunk_pos = {cam_to_chunk.x + i, cam_to_chunk.y + j};

        chunk *ch = lmap_at(&w->chunks, &chunk_pos);

        if (!ch) {
          chunk gen = chunk_new(w, chunk_pos);
          ch = lmap_add(&w->chunks, &chunk_pos, &gen);
        }

        arr_add_arr(&w->vb_cache, ch->data, chunk_len * chunk_len,
                    sizeof(obj_vtx));
      }
    }

    if (nc > last_nc) {
      for (int i = last_nc; i < nc; i++) {
        for (int j = 0; j < n_inds; j++) {
          arr_add(&w->ib_cache, &(int){qinds[j] + i * (chunk_len * chunk_len)});
        }
      }

      w->ib_dirty = 1;
    }

    last_nc = nc;

    w->last_chunk_pos = cam_to_chunk;
    w->vb_dirty = 1;
  }
#undef n_inds
}

void world_draw(world *w, draw_src s, cam *c, float d) {
  static mtl chunk_mtl = {
    .light = 6,
    .dark = 0,
    .light_model = {0, 0.8f, 0}
  };

  mod_get_sh(s, c, chunk_mtl, m4_ident);

  if (w->vb_dirty) {
    buf_data_n(&w->vb, GL_DYNAMIC_DRAW, sizeof(obj_vtx), arr_len(w->vb_cache),
               w->vb_cache);
    w->vb_dirty = 0;
  }

  if (w->ib_dirty) {
    buf_data_n(&w->ib, GL_DYNAMIC_DRAW, sizeof(int), arr_len(w->ib_cache),
               w->ib_cache);
    w->ib_dirty = 0;
  }

  vao_bind(&w->va);
  size_t n_inds = arr_len(w->ib_cache);
  gl_draw_elements(GL_TRIANGLES, n_inds, GL_UNSIGNED_INT, 0);
  the_app.n_tris += n_inds / 3;

  the_app.n_drawn = the_app.n_close = 0;

  for (obj *o = w->objs, *end = arr_end(w->objs); o != end; o++) {
    body *b = &o->body;
    if (v3_dist(b->pos, c->pos) > (world_draw_dist + 1) * chunk_size) {
      continue;
    }

    the_app.n_close++;

    if (!cam_test_box(&the_app.cam, obj_get_box(o), s)) {
      continue;
    }

    the_app.n_drawn++;

    obj_draw(o, s, c, d);
  }
}

void world_add_obj(world *w, obj *o) {
  o->body.prev_pos = o->body.pos;
  o->world = w;
  o->id = arr_len(w->objs_tick);
  arr_add(&w->objs_to_add, o);
}