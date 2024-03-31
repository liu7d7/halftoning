#include "world.h"
#include "typedefs.h"
#include "obj.h"

float chunk_get_y(v3f world_pos) {
  static fnl_state *noise = NULL;
  if (!noise) {
    fnl_state s = fnlCreateState();
    noise = memcpy(malloc(sizeof(s)), &s, sizeof(s));
    noise->noise_type = FNL_NOISE_OPENSIMPLEX2S;
  }

  return fnlGetNoise2D(noise, world_pos.x * 12, world_pos.z * 12) * 1.5f +
         fnlGetNoise2D(noise, world_pos.x, world_pos.z) * 8.5f;
}

v3f chunk_get_pos(v2i pos, int off_x, int off_z) {
  v3f base = {
    (float)pos.x * (float)chunk_size + (float)off_x * chunk_ratio,
    0,
    (float)pos.y * (float)chunk_size + (float)off_z * chunk_ratio
  };

  base.y = chunk_get_y(base);
  return base;
}

chunk chunk_new(v2i pos) {
  chunk_vtx *verts = arr_new(chunk_vtx, 4);
  tri *tris = arr_new(tri, 4);

  for (int i = 0; i < chunk_len; i++) {
    for (int j = 0; j < chunk_len; j++) {
      v3f
        a = chunk_get_pos(pos, i, j),
        b = chunk_get_pos(pos, i, j + 1),
        c = chunk_get_pos(pos, i + 1, j + 1),
        d = chunk_get_pos(pos, i + 1, j);

      v3f n_abc = v3_normed(v3_cross(v3_sub(b, a), v3_sub(c, a)));
      v3f n_acd = v3_normed(v3_cross(v3_sub(c, a), v3_sub(d, a)));

      arr_add(&verts, &(chunk_vtx){.pos = a, .norm = n_abc});
      arr_add(&verts, &(chunk_vtx){.pos = b, .norm = n_abc});
      arr_add(&verts, &(chunk_vtx){.pos = c, .norm = n_abc});

      tri t = tri_new(a, b, c, n_abc, 0);
      arr_add(&tris, &t);

      arr_add(&verts, &(chunk_vtx){.pos = a, .norm = n_acd});
      arr_add(&verts, &(chunk_vtx){.pos = c, .norm = n_acd});
      arr_add(&verts, &(chunk_vtx){.pos = d, .norm = n_acd});

      t = tri_new(a, c, d, n_acd, 0);
      arr_add(&tris, &t);
    }
  }

  buf vbo = buf_new(GL_ARRAY_BUFFER);
  buf_data_n(&vbo, GL_DYNAMIC_DRAW, sizeof(chunk_vtx),
             arr_len(verts), verts);

  chunk c = {
    .vao = vao_new(&vbo, NULL, 3, (attrib[]){attr_3f, attr_3f, attr_2f}),
    .pos = pos,
    .n_inds = arr_len(verts),
    .obj = (obj){.m = tmesh_new(tris)}
  };

  arr_del(verts);

  return c;
}

world world_new() {
  world w = {
    .chunks = map_new(16, sizeof(v2i), sizeof(chunk), 0.75f, iv2_eq, iv2_hash),
    .objs = arr_new(obj, 4)
  };

  for (int i = -world_draw_dist * 2; i <= world_draw_dist * 2; i++) {
    for (int j = -world_draw_dist * 2; j <= world_draw_dist * 2; j++) {
      chunk c = chunk_new((v2i){i, j});
      map_add(&w.chunks, &(v2i){i, j}, &c);
    }
  }

  for (int i = 0; i < 64; i++) {
    arr_add(&w.objs, &(obj){.c = cap_new((v3f){4.f * (i % 8), 30, 4.f * (i / 8)}, v3_uy, 0.5f, 3.f)});
  }

  for (int i = 0; i < world_sp_size; i++) {
    for (int j = 0; j < world_sp_size; j++) {
      w.regions[i][j] = reg_new();
    }
  }

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

      chunk *ch = map_at(&w->chunks, &chunk_pos);
      if (!ch) continue;

      reg *r =
        &w->regions[i + world_draw_dist][j + world_draw_dist];

      reg_add_sta(r, &ch->obj);
    }
  }

  // handle all objects
  for (obj *o = w->objs; o != (obj *)arr_end(w->objs); o++) {
    *obj_get_prev_pos(o) = *obj_get_pos(o);
    v2i obj_pos = world_get_chunk_pos(*obj_get_pos(o));
    v2i off = iv2_sub(obj_pos, cam_pos);
    for (int i = -1; i <= 1; i++) {
      for (int j = -1; j <= 1; j++) {
        if (abs(off.x + i) <= world_draw_dist &&
            abs(off.y + j) <= world_draw_dist) {
          reg *r = &w->regions[off.x + i + world_draw_dist][off.y + j +
                                                            world_draw_dist];
          reg_add_dyn(r, o);
        }
      }
    }
  }

  // tick each region
  int const sub_steps = 4;
  float const step_time = 1.f / (float)sub_steps;

  for (int t = 0; t < sub_steps; t++) {
    for (obj *o = w->objs; o != arr_end(w->objs); o++) {
      obj_tick(o, step_time);
    }

    for (int i = 0; i < world_sp_size; i++) {
      for (int j = 0; j < world_sp_size; j++) {
        reg *s = &w->regions[i][j];
        if (!reg_is_tickable(s)) continue;

        reg_tick(s);
      }
    }
  }
}

void world_draw(world *w, cam *c, float d) {
  v2i cam_to_chunk = world_get_chunk_pos(c->pos);

  (void)mod_get_shader(c, m4_ident, d);

  int n_gen = 0;
  for (int i = -world_draw_dist; i <= world_draw_dist; i++) {
    for (int j = -world_draw_dist; j <= world_draw_dist; j++) {
      if (sqrt(i * i + j * j) > world_draw_dist + 1) continue;

      v2i chunk_pos = {cam_to_chunk.x + i, cam_to_chunk.y + j};

      if (!map_has(&w->chunks, &chunk_pos) && n_gen < 4) {
        chunk gen = chunk_new(chunk_pos);
        map_add(&w->chunks, &chunk_pos, &gen);
        n_gen++;
      }

      chunk *ch = map_at(&w->chunks, &chunk_pos);
      if (!ch) continue;

      vao_bind(&ch->vao);
      gl_draw_arrays(GL_TRIANGLES, 0, ch->n_inds);
    }
  }

  for (int i = 0; i < world_sp_size; i++) {
    for (int j = 0; j < world_sp_size; j++) {
      reg *r = &w->regions[i][j];
      for (obj **op = r->dyn; op != arr_end(r->dyn); op++) {
        obj_draw(*op, c, d);
      }

      for (obj *o = r->sta; o != arr_end(r->sta); o++) {
        obj_draw(o, c, d);
      }
    }
  }
}
