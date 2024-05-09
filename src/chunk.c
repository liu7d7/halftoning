#include "chunk.h"
#include "arr.h"
#include "world.h"
#include "lib/simplex/FastNoiseLite.h"

float chunk_get_y(v3f world_pos) {
  static fnl_state *noise = NULL;
  if (!noise) {
    fnl_state s = fnlCreateState();
    noise = memcpy(malloc(sizeof(s)), &s, sizeof(s));
    noise->noise_type = FNL_NOISE_OPENSIMPLEX2S;
  }

  return fnlGetNoise2D(noise, world_pos.x * 9 + chunk_sizef * -63.f, world_pos.z * 9 + chunk_sizef * 48.f) * 0.5f +
         fnlGetNoise2D(noise, world_pos.x * 2.f + chunk_sizef * 15.f, world_pos.z * 2.f + chunk_sizef * 15.f) * 4.5f +
         fnlGetNoise2D(noise, world_pos.x, world_pos.z) * 9.5f +
         fnlGetNoise2D(noise, world_pos.x * 0.25f - 112, world_pos.z * 0.25f + 32) * 18.5f;
}

v3f chunk_get_pos(v2i pos, int off_x, int off_z) {
  v3f base = {
    (float)pos.x * chunk_sizef + (float)off_x * chunk_ratio,
    0,
    (float)pos.y * chunk_sizef + (float)off_z * chunk_ratio
  };

  base.y = chunk_get_y(base);
  return base;
}

v3f chunk_get_posf(v2i pos, float off_x, float off_z) {
  v3f base = {
    (float)pos.x * chunk_sizef + off_x * chunk_ratio,
    0,
    (float)pos.y * chunk_sizef + off_z * chunk_ratio
  };

  base.y = chunk_get_y(base);
  return base;
}

v3f norm_at(v2i pos, float i, float j) {
  v3f a = chunk_get_posf(pos, i, j), b = chunk_get_posf(pos, i + 0.01f, j), c = chunk_get_posf(pos, i, j + 0.01f);
  return v3_normed(v3_cross(v3_sub(c, a), v3_sub(b, a)));
}

void chunk_build_normals(v2i pos, obj_vtx *verts) {
  static v2i norm_offsets[] = {
    (v2i){1, 0}, (v2i){1, 1}, (v2i){0, 1}
  };

  for (int i = 0; i < chunk_len; i++) {
    for (int j = 0; j < chunk_len; j++) {
      verts[i * chunk_len + j].norm = norm_at(pos, i, j);
    }
  }
}

chunk chunk_new(world *w, v2i pos) {
  static int *inds = NULL;
  if (!inds) {
    inds = quad_indices(chunk_len, chunk_len);
  }

  obj_vtx verts[chunk_len * chunk_len];
  for (int i = 0; i < chunk_len; i++) {
    for (int j = 0; j < chunk_len; j++) {
      verts[i * chunk_len + j].pos = chunk_get_pos(pos, i, j);
      verts[i * chunk_len + j].norm = v3_zero;
    }
  }

  chunk_build_normals(pos, verts);

  body phys = (body){.mesh = tmesh_new_vi(verts, inds), .slip = 0.99f};

  chunk c = {
    .body = phys
  };

  memcpy(c.data, verts, chunk_len * chunk_len * sizeof(obj_vtx));

  if (rndf(0, 1) > 0.45) {
    float xo = rndf(0, chunk_size), zo = rndf(0, chunk_size);
    obj t = tree_new(chunk_get_posf(pos, xo, zo), norm_at(pos, xo, zo));
    world_add_obj(w, &t);
  }

  return c;
}