#pragma once

#include "typedefs.h"
#include "gl.h"
#include "map.h"
#include "pal.h"

#define ani_bones_per_vtx 4

typedef struct ani_vtx {
  v3 pos;
  v3 norm;
  int bones[4];
  float weights[4];
} ani_vtx;

typedef struct ani_mesh {
  ani_vtx *vtxs;
  int n_vtxs;
  int n_inds;
  mtl mat;

  buf vbo, ibo;
  vao vao;
  char const *name;
} ani_mesh;

typedef struct ani_mod {
  tex *texes;
  int n_texes;

  ani_mesh *meshes;
  int n_meshes;

  box3 bounds;

  map bone_info_map;
  int n_bones;
} ani_mod;

typedef struct bone_info {
  int id;
  m4 off;
} bone_info;

typedef struct key_pos {
  v3 pos;
  float time;
} key_pos;

typedef struct key_rot {
  // TODO: make a proper quat class!
  v4 rot;
  float time;
} key_rot;

typedef struct key_scale {
  v3 scale;
  float time;
} key_scale;

typedef struct bone {
  key_pos *pos;
  key_rot *rot;
  key_scale *scale;

  m4 to_local;
  char const *name;
  int id;
} bone;

bone bone_new(char const *name, int id, struct aiNodeAnim const *channel);

void bone_tick(bone *b, float t);

typedef struct ani_node {
  m4 trans;
  char const *name;
  struct ani_node *children;
} ani_node;

typedef struct animation {
  float duration;
  int tps;

  // TODO: can this be a map?
  bone *bones;
  ani_node root_node;
  map bone_info_map;
} animation;

animation animation_new(char const *path, ani_mod *model);

typedef struct anime {
  m4 *final_mats;
  animation cur;
  float t;
  float dt;
} anime;

extern anime ani_dummy;

anime anime_new(animation animation);

anime anime_tick(anime *a, float dt);

void anime_play(anime *a, animation anim);

ani_mod ani_mod_new(char const *path);

shdr *ani_mod_get_sh(draw_src s, cam *c, anime *a, mtl m, m4 t);

void ani_mod_draw(ani_mod *m, anime *a, draw_src s, cam *c, m4 t, int id);