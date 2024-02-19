#include "world.h"

float chunk_get_y(const float* world_pos) {
  static fnl_state* noise = NULL;
  if (!noise) {
    fnl_state s = fnlCreateState();
    noise = memcpy(malloc(sizeof(s)), &s, sizeof(s));
    noise->noise_type = FNL_NOISE_OPENSIMPLEX2;
  }

  return fnlGetNoise2D(noise, world_pos[0] * 2, world_pos[2] * 2) * 15;
}

void chunk_get_pos(const int* pos, int off_x, int off_z, float* out) {
  vec3 base = {
    (float)(pos[0] * chunk_size + off_x * chunk_ratio),
    0,
    (float)(pos[1] * chunk_size + off_z * chunk_ratio)
  };

  base[1] = chunk_get_y(base);
  vec3cpy(out, base);
}

struct chunk chunk(int* pos) {
  struct chunk_vtx* verts = dyn_arr(struct chunk_vtx, 4);

  for (int i = 0; i < chunk_len; i++) {
    for (int j = 0; j < chunk_len; j++) {
      vec3 a, b, c, d, ba, ca, da, n_abc, n_acd;
      chunk_get_pos(pos, i, j, a);
      chunk_get_pos(pos, i, j + 1, b);
      chunk_get_pos(pos, i + 1, j + 1, c);
      chunk_get_pos(pos, i + 1, j, d);

      glm_vec3_sub(b, a, ba);
      glm_vec3_sub(c, a, ca);
      glm_vec3_sub(d, a, da);

      glm_cross(ba, ca, n_abc);
      glm_cross(ca, da, n_acd);
      glm_normalize(n_abc);
      glm_normalize(n_acd);

      dyn_arr_add(verts, &(struct chunk_vtx){
        .pos = VEC3_COPY_INIT(a),
        .norm = VEC3_COPY_INIT(n_abc)
      });
      dyn_arr_add(verts, &(struct chunk_vtx){
        .pos = VEC3_COPY_INIT(b),
        .norm = VEC3_COPY_INIT(n_abc)
      });
      dyn_arr_add(verts, &(struct chunk_vtx){
        .pos = VEC3_COPY_INIT(c),
        .norm = VEC3_COPY_INIT(n_abc)
      });

      dyn_arr_add(verts, &(struct chunk_vtx){
        .pos = VEC3_COPY_INIT(a),
        .norm = VEC3_COPY_INIT(n_acd)
      });
      dyn_arr_add(verts, &(struct chunk_vtx){
        .pos = VEC3_COPY_INIT(c),
        .norm = VEC3_COPY_INIT(n_acd)
      });
      dyn_arr_add(verts, &(struct chunk_vtx){
        .pos = VEC3_COPY_INIT(d),
        .norm = VEC3_COPY_INIT(n_acd)
      });
    }
  }

  struct buf vbo = buf(GL_ARRAY_BUFFER);
  buf_data_n(&vbo, GL_DYNAMIC_STORAGE_BIT, sizeof(struct chunk_vtx),
             dyn_arr_count(verts), verts);

  struct chunk c = {
    .vao = vao(&vbo, NULL, 3, (struct attrib[]){attr_3f, attr_3f, attr_2f}),
    .pos = VEC2_COPY_INIT(pos),
    .n_inds = dyn_arr_count(verts)
  };

  dyn_arr_del(verts);

  return c;
}

struct world world() {
  return (struct world) {
    .chunks = map(16, sizeof(ivec2), sizeof(struct chunk), 0.75f, ivec2_eq, ivec2_hash)
  };
}

void world_get_chunk_pos(const float* world_pos, int* out) {
  out[0] = (int)(world_pos[0] / (float)chunk_size);
  out[1] = (int)(world_pos[2] / (float)chunk_size);
}

void world_draw(struct world* w, struct cam* c) {
  ivec2 cam_to_chunk;
  world_get_chunk_pos(c->pos, cam_to_chunk);

  (void)mod_get_shader(c);

  const int render_distance = 6;
  for (int i = -render_distance; i <= render_distance; i++) {
    for (int j = -render_distance; j <= render_distance; j++) {
      ivec2 chunk_pos = {cam_to_chunk[0] + i, cam_to_chunk[1] + j};
      if (!map_has(&w->chunks, &chunk_pos)) {
        struct chunk gen = chunk(chunk_pos);
        map_set(&w->chunks, &chunk_pos, &gen);
      }

      struct chunk* ch = map_at(&w->chunks, &chunk_pos);
      vao_bind(&ch->vao);
      gl_draw_arrays(GL_TRIANGLES, 0, ch->n_inds);
    }
  }
}
