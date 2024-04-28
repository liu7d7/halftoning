#include "chunk.h"
#include "arr.h"
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

void chunk_build_normals(v2i pos, obj_vtx *verts) {
  static v2i norm_offsets[] = {
    (v2i){1, 0}, (v2i){1, 1}, (v2i){0, 1}
  };

  for (int i = 0; i < chunk_len; i++) {
    for (int j = 0; j < chunk_len; j++) {
      v3f a = chunk_get_pos(pos, i, j), b = chunk_get_posf(pos, i + 0.01f, j), c = chunk_get_posf(pos, i, j + 0.01f);
      verts[i * chunk_len + j].norm = v3_normed(v3_cross(v3_sub(c, a), v3_sub(b, a)));
    }
  }
}

chunk chunk_new(v2i pos) {
  obj_vtx verts[chunk_len * chunk_len];
  for (int i = 0; i < chunk_len; i++) {
    for (int j = 0; j < chunk_len; j++) {
      verts[i * chunk_len + j].pos = chunk_get_pos(pos, i, j);
      verts[i * chunk_len + j].norm = v3_zero;
    }
  }

  chunk_build_normals(pos, verts);

  int *inds = quad_indices(chunk_len, chunk_len);
  buf vbo = buf_new(GL_ARRAY_BUFFER), ibo = buf_new(GL_ELEMENT_ARRAY_BUFFER);
  buf_data_n(&vbo, GL_STATIC_DRAW, sizeof(obj_vtx), chunk_len * chunk_len, verts);

  buf_data_n(&ibo, GL_STATIC_DRAW, sizeof(int), arr_len(inds), inds);
  body phys = (body){.mesh = tmesh_new_vi(verts, inds), .slip = 0.99f};
  int n_inds = arr_len(inds);
  arr_del(inds);

  return (chunk){
    .pos = pos,
    .vao = vao_new(&vbo, &ibo, 2, (attrib[]){attr_3f, attr_3f}),
    .n_inds = n_inds,
    .body = phys
  };
}