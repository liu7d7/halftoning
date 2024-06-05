#include "chunk.h"
#include "arr.h"
#include "world.h"
#include "lib/simplex/FastNoiseLite.h"
#include "app.h"
#include "pal.h"

float chunk_get_y(v3 world_pos) {
  static fnl_state *noise = NULL;
  if (!noise) {
    noise = _new_(fnlCreateState());
    noise->noise_type = FNL_NOISE_OPENSIMPLEX2S;
  }

  return fnlGetNoise2D(noise, world_pos.x * 9 + chunk_sizef * -63.f, world_pos.z * 9 + chunk_sizef * 48.f) * 0.5f +
         fnlGetNoise2D(noise, world_pos.x * 2.f + chunk_sizef * 15.f, world_pos.z * 2.f + chunk_sizef * 15.f) * 4.5f +
         fnlGetNoise2D(noise, world_pos.x, world_pos.z) * 9.5f +
         fnlGetNoise2D(noise, world_pos.x * 0.25f - 112, world_pos.z * 0.25f + 32) * 18.5f;
}

v3 chunk_get_pos(iv2 pos, int off_x, int off_z) {
  v3 base = {
    (float)pos.x * chunk_sizef + (float)off_x * chunk_ratio,
    0,
    (float)pos.y * chunk_sizef + (float)off_z * chunk_ratio
  };

  base.y = chunk_get_y(base);
  return base;
}

v3 chunk_get_posf(iv2 pos, float off_x, float off_z) {
  v3 base = {
    (float)pos.x * chunk_sizef + off_x * chunk_ratio,
    0,
    (float)pos.y * chunk_sizef + off_z * chunk_ratio
  };

  base.y = chunk_get_y(base);
  return base;
}

v3 norm_at(iv2 pos, float i, float j) {
  v3 a = chunk_get_posf(pos, i, j), b = chunk_get_posf(pos, i + 0.01f, j), c = chunk_get_posf(pos, i, j + 0.01f);
  return v3_normed(v3_cross(v3_sub(c, a), v3_sub(b, a)));
}

void chunk_build_normals(iv2 pos, ch_vtx *verts) {
  static iv2 norm_offsets[] = {
    (iv2){1, 0}, (iv2){1, 1}, (iv2){0, 1}
  };

  for (int i = 0; i < chunk_len; i++) {
    for (int j = 0; j < chunk_len; j++) {
      verts[i * chunk_len + j].norm = norm_at(pos, i, j);
    }
  }
}

chunk chunk_new(world *w, iv2 pos) {
  static int *inds = NULL;
  if (!inds) {
    inds = quad_indices(chunk_len, chunk_len);
  }

  int id = rndi(INT_MIN, INT_MAX);

  ch_vtx verts[chunk_len * chunk_len];
  for (int i = 0; i < chunk_len; i++) {
    for (int j = 0; j < chunk_len; j++) {
      verts[i * chunk_len + j].pos = chunk_get_pos(pos, i, j);
      verts[i * chunk_len + j].norm = v3_zero;
      verts[i * chunk_len + j].id = id;
    }
  }

  chunk_build_normals(pos, verts);

  body phys = (body){.mesh = tmesh_new_cvi(verts, inds), .slip = 0.99f};

  chunk c = {
    .body = phys,
  };

  memcpy(c.data, verts, chunk_len * chunk_len * sizeof(ch_vtx));

  if (rndf(0, 1) > 0.4) {
    float xo = rndf(0, chunk_size), zo = rndf(0, chunk_size);
    obj t = tree_new(chunk_get_posf(pos, xo, zo), norm_at(pos, xo, zo));
    world_add_obj(w, &t);
  }

  return c;
}

shdr *ch_get_sh(draw_src s, cam *c) {
  static shdr *cam = NULL;
  static shdr *shade = NULL;
  static mtl m = {
    .light = 6,
    .dark = 0,
    .light_model = {0, 0.8f, 0}
  };

  if (!cam) {
    cam = _new_(shdr_new(2,
                         (shdr_spec[]){
                           {GL_VERTEX_SHADER,   "res/chunk.vsh"},
                           {GL_FRAGMENT_SHADER, "res/mod_light.fsh"},
                         }));

    shade = _new_(shdr_new(2,
                           (shdr_spec[]){
                             {GL_VERTEX_SHADER,   "res/mod_depth.vsh"},
                             {GL_FRAGMENT_SHADER, "res/mod_depth.fsh"},
                           }));
  }

  shdr *cur = s == ds_cam ? cam : shade;

  shdr_m4f(cur, "u_vp", c->vp);
  shdr_m4f(cur, "u_model", m4_ident);
  shdr_3f(cur, "u_eye", cam_get_eye(c));
  shdr_1f(cur, "u_time", app_now() / 1000.f);
  shdr_3f(cur, "u_light_model", m.light_model);
  shdr_3f(cur, "u_light", dreamy_haze[m.light]);
  shdr_3f(cur, "u_dark", dreamy_haze[m.dark]);
  shdr_1f(cur, "u_trans", m.transmission);
  shdr_1f(cur, "u_shine", m.shine);
  shdr_1f(cur, "u_wind", m.wind);
  shdr_bind(cur);

  return cur;
}
