#include "ani.h"
#include "app.h"

bone bone_new(char const *name, int id, struct aiNodeAnim const *channel) {
  key_pos *pos = arr_new(key_pos);
  key_rot *rot = arr_new(key_rot);
  key_scale *scale = arr_new(key_scale);
  for (int i = 0; i < channel->mNumPositionKeys; i++) {
    v3 posi = *(v3 *)&channel->mPositionKeys[i].mValue;
    float time = channel->mPositionKeys[i].mTime;
    arr_add(&pos, &(key_pos){.pos = posi, .time = time});
  }

  for (int i = 0; i < channel->mNumRotationKeys; i++) {
    struct aiQuaternion quat = channel->mRotationKeys[i].mValue;
    v4 rota = {quat.x, quat.y, quat.z, quat.w};
    float time = channel->mRotationKeys[i].mTime;
    arr_add(&rot, &(key_rot){.rot = rota, .time = time});
  }

  for (int i = 0; i < channel->mNumScalingKeys; i++) {
    v3 scaling = *(v3 *)&channel->mScalingKeys[i].mValue;
    float time = channel->mScalingKeys[i].mTime;
    arr_add(&scale, &(key_scale){.scale = scaling, .time = time});
  }

  return (bone){
    .name = name,
    .id = id,
    .to_local = m4_ident,
    .scale = scale,
    .rot = rot,
    .pos = pos
  };
}

float get_scale_factor(float last, float next, float t) {
  float delta = t - last;
  float total = next - last;
  return delta / total;
}

/* private */ int get_pos_idx(bone *b, float t) {
  for (int i = 0; i < arr_len(b->pos) - 1; i++) {
    if (t < b->pos[i + 1].time) {
      return i;
    }
  }

  throwf("get_pos_idx failed on %s at %f", b->name, t);
}

/* private */ m4 interp_pos(bone *b, float t) {
  if (arr_len(b->pos) == 1) {
    return m4_trans_v(b->pos->pos);
  }

  int i0 = get_pos_idx(b, t);
  int i1 = i0 + 1;
  float d = get_scale_factor(b->pos[i0].time, b->pos[i1].time, t);
  v3 out = v3_lerp(b->pos[i0].pos, b->pos[i1].pos, d);
  return m4_trans_v(out);
}

/* private */ int get_rot_idx(bone *b, float t) {
  for (int i = 0; i < arr_len(b->rot) - 1; i++) {
    if (t < b->rot[i + 1].time) {
      return i;
    }
  }

  throwf("get_rot_idx failed on %s at %f", b->name, t);
}

/* private */ m4 interp_rot(bone *b, float t) {
  if (arr_len(b->rot) == 1) {
    return v4q_to_mat(b->rot->rot);
  }

  int i0 = get_pos_idx(b, t);
  int i1 = i0 + 1;
  float d = get_scale_factor(b->rot[i0].time, b->rot[i1].time, t);
  return v4q_to_mat(v4q_slerp(b->rot[i0].rot, b->rot[i1].rot, d));
}

/* private */ int get_scale_idx(bone *b, float t) {
  for (int i = 0; i < arr_len(b->scale) - 1; i++) {
    if (t < b->scale[i + 1].time) {
      return i;
    }
  }

  throwf("get_scale_idx failed on %s at %f", b->name, t);
}

/* private */ m4 interp_scale(bone *b, float t) {
  if (arr_len(b->scale) == 1) {
    return m4_scale_v(b->scale->scale);
  }

  int i0 = get_scale_idx(b, t);
  int i1 = i0 + 1;
  float d = get_scale_factor(b->scale[i0].time, b->scale[i1].time, t);
  v3 out = v3_lerp(b->scale[i0].scale, b->scale[i1].scale, d);
  return m4_scale_v(out);
}

void bone_tick(bone *b, float t) {
  m4 pos = interp_pos(b, t);
  m4 rot = interp_rot(b, t);
  m4 scale = interp_scale(b, t);
  b->to_local = m4_mul(rot, m4_mul(scale, pos));
}

/* private */ void read_hierarchy_data(ani_node *n, struct aiNode *a) {
  n->name = strdup(a->mName.data);
  n->trans = m4_tpose(*(m4 *)&a->mTransformation);
  n->children = arr_new(ani_node);
  for (int i = 0; i < a->mNumChildren; i++) {
    ani_node child;
    read_hierarchy_data(&child, a->mChildren[i]);
    arr_add(&n->children, &child);
  }
}

/* private */
void read_missing_bones(animation *a, ani_mod *m, struct aiAnimation const *anim) {
  int size = anim->mNumChannels;

  for (int i = 0; i < size; i++) {
    struct aiNodeAnim *channel = anim->mChannels[i];
    char const *name = channel->mNodeName.data;

    bone_info *b = map_at(&m->bone_info_map, &name);
    if (!b) {
      b = map_add(&m->bone_info_map, &name, &(bone_info){.id = m->n_bones++, .off = m4_ident});
    }

    bone bone = bone_new(name, b->id, channel);
    arr_add(&a->bones, &bone);
  }

  a->bone_info_map = m->bone_info_map;
}

animation animation_new(const char *path, ani_mod *model) {
  struct aiScene const *scene = aiImportFile(path, aiProcessPreset_TargetRealtime_Fast);
  if (scene == NULL || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    throwf(aiGetErrorString());
  }

  struct aiAnimation *anim = scene->mAnimations[0];
  float duration = anim->mDuration;
  int tps = anim->mTicksPerSecond;
  animation out = {
    .duration = duration,
    .tps = tps,
    .bones = arr_new(bone)
  };

  read_hierarchy_data(&out.root_node, scene->mRootNode);
  read_missing_bones(&out, model, anim);

  // TODO: figure out whether it's safe to release import!

  return out;
}

anime anime_new(animation animation) {
  anime out = {
    .t = 0.f,
    .dt = 0.f,
    .cur = animation,
    .final_mats = arr_new_sized(m4, 100)
  };

  for (int i = 0; i < 100; i++) {
    arr_add(&out.final_mats, &m4_ident);
  }

  return out;
}

/* private */
void calc_bone_trans(anime *a, ani_node *node, m4 parent) {
  char const *name = node->name;
  m4 trans = node->trans;

  bone *b;
  for (b = a->cur.bones; b != arr_end(a->cur.bones); b++) {
    if (strcmp(b->name, name) == 0) break;
  }

  if (b != arr_end(a->cur.bones)) {
    bone_tick(b, a->t);
    trans = b->to_local;
  }

  m4 global = m4_mul(trans, parent);

  bone_info *inf = map_at(&a->cur.bone_info_map, &name);
  if (inf) {
    int idx = inf->id;
    m4 off = inf->off;
    a->final_mats[idx] = m4_mul(off, global);
  }

  for (ani_node *n = node->children, *e = arr_end(n); n != e; n++) {
    calc_bone_trans(a, n, global);
  }
}

anime anime_tick(anime *a, float dt) {
  a->dt = dt;
  a->t += dt * a->cur.tps;
  a->t = fmodf(a->t, a->cur.duration);
  calc_bone_trans(a, &a->cur.root_node, m4_ident);
}

void anime_play(anime *a, animation anim) {
  a->cur = anim;
  a->t = 0.f;
}

/* private */ void set_bone_weights(ani_mod *m, ani_vtx *v, struct aiMesh *mesh,
                                    struct aiScene const *scene) {
  for (int bone_idx = 0; bone_idx < mesh->mNumBones; bone_idx++) {
    char const *bone_name = strdup(mesh->mBones[bone_idx]->mName.data);
    bone_info *inf = map_at(&m->bone_info_map, &bone_name);
    if (inf == NULL) {
      bone_info bi = {
        .id = m->n_bones++,
        .off = m4_tpose(*(m4 *)&mesh->mBones[bone_idx]->mOffsetMatrix)
      };

      inf = map_add(&m->bone_info_map, &bone_name, &bi);
    }

    struct aiVertexWeight *weights = mesh->mBones[bone_idx]->mWeights;
    int n_weights = mesh->mBones[bone_idx]->mNumWeights;

    for (int weight_idx = 0; weight_idx < n_weights; weight_idx++) {
      int v_id = weights[weight_idx].mVertexId;
      float v_weight = weights[weight_idx].mWeight;
      for (int i = 0; i < ani_bones_per_vtx; i++) {
        if (v[v_id].bones[i] != -1) continue;
        v[v_id].bones[i] = inf->id;
        v[v_id].weights[i] = v_weight;
        break;
      }
    }
  }
}

/* private */ ani_mesh
ani_mod_load_mesh(ani_mod *m, map *mats, box3 *b, struct aiMesh *mesh,
                  const struct aiScene *scene) {
  ani_vtx *vtxs = malloc(sizeof(ani_vtx) * mesh->mNumVertices);

  size_t len = strlen(mesh->mName.data);

  char *name = mesh->mName.data;
  char name_thing[1024];
  strcpy(name_thing, name);
  char *dot_ptr = strchr(name, '.');
  if (dot_ptr) {
    size_t dot = dot_ptr - name;
    name_thing[dot] = '\0';
  }

  char *name_thing_ptr = name_thing;
  mtl *mat = map_at(mats, &name_thing_ptr);
  if (!mat) {
    throwf("mod_load_mesh: failed to find material %s in map!", mesh->mName.data);
  }

  for (int i = 0; i < mesh->mNumVertices; i++) {
    v3 pos = *(v3 *)&mesh->mVertices[i], norm = *(v3 *)&mesh->mNormals[i];

    vtxs[i] = (ani_vtx){
      .pos = pos,
      .norm = norm,
      .bones = {-1, -1, -1, -1}};

    b->min = v3_min(b->min, vtxs[i].pos);
    b->max = v3_max(b->max, vtxs[i].pos);
  }

  set_bone_weights(m, vtxs, mesh, scene);

  buf vbo = buf_new(GL_ARRAY_BUFFER), ibo = buf_new(GL_ELEMENT_ARRAY_BUFFER);

  buf_data_n(&vbo,
             GL_DYNAMIC_DRAW,
             sizeof(ani_vtx),
             mesh->mNumVertices,
             vtxs);

  u32 *inds = arr_new(u32);
  for (int i = 0; i < mesh->mNumFaces; i++) {
    for (int j = 0; j < mesh->mFaces[i].mNumIndices; j++) {
      arr_add(&inds, &mesh->mFaces[i].mIndices[j]);
    }
  }

  buf_data_n(&ibo,
             GL_DYNAMIC_DRAW,
             sizeof(u32),
             arr_len(inds),
             inds);

  char *heap_name = malloc(len + 1);
  strcpy_s(heap_name, len + 1, name);

  struct ani_mesh me = {
    .vtxs = vtxs,
    .n_vtxs = (int)mesh->mNumVertices,
    .n_inds = arr_len(inds),
    .vao = vao_new(&vbo, &ibo, 4,
                   (attrib[]){attr_3f, attr_3f, attr_4i, attr_4f}),
    .mat = *mat,
    .name = heap_name,
    .ibo = ibo,
    .vbo = vbo
  };

  arr_del(inds);

  return me;
}

/* private */ void
ani_mod_load(ani_mod *m, map *mats, box3 *b, struct aiNode *node,
             const struct aiScene *scene) {
  for (int i = 0; i < node->mNumMeshes; i++) {
    struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    m->meshes[m->n_meshes++] = ani_mod_load_mesh(m, mats, b, mesh, scene);
  }

  for (int i = 0; i < node->mNumChildren; i++) {
    ani_mod_load(m, mats, b, node->mChildren[i], scene);
  }
}

/* private */ ani_mod ani_mod_from_scene(struct aiScene const *scene, const char *path) {
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    throwf(aiGetErrorString());
  }

  ani_mod m = {
    .meshes = malloc(sizeof(mesh) * scene->mNumMeshes),
    .bone_info_map = map_new(4, sizeof(char const *), sizeof(bone_info), 0.75f, str_eq, str_hash)
  };

  map mats = mod_load_mtl(path);

  float large = 1e20f;
  box3 b = box3_new((v3){large, large, large}, (v3){-large, -large, -large});

  ani_mod_load(&m, &mats, &b, scene->mRootNode, scene);
  m.bounds = b;

  aiReleaseImport(scene);

  return m;
}

ani_mod ani_mod_new(const char *path) {
  struct aiScene const *scene =
    aiImportFile(path, aiProcessPreset_TargetRealtime_Fast);

  return ani_mod_from_scene(scene, path);
}

shdr *ani_mod_get_sh(draw_src s, cam *c, anime *a, mtl m, m4 t) {
  static shdr *cam = NULL;
  static shdr *shade = NULL;
  if (!cam) {
    cam = _new_(shdr_new(2,
                         (shdr_s[]){
                           {GL_VERTEX_SHADER,   "res/ani_mod.vsh"},
                           {GL_FRAGMENT_SHADER, "res/mod_light.fsh"},
                         }));

    shade = _new_(shdr_new(2,
                           (shdr_s[]){
                             {GL_VERTEX_SHADER,   "res/ani_mod_depth.vsh"},
                             {GL_FRAGMENT_SHADER, "res/mod_depth.fsh"},
                           }));
  }

  shdr *cur = s == ds_cam ? cam : shade;

  shdr_m4f(cur, "u_vp", c->vp);
  shdr_3f(cur, "u_eye", cam_get_eye(c));
  shdr_1f(cur, "u_time", app_now() / 1000.f);
  shdr_m4f(cur, "u_model", t);
  gl_program_uniform_matrix_4fv(cur->id, shdr_get_loc(cur, "u_final_mats"), 100, GL_TRUE, (float *)a->final_mats);
  shdr_3f(cur, "u_light_model", m.light_model);
  shdr_3f(cur, "u_light", dreamy_haze[m.light]);
  shdr_3f(cur, "u_dark", dreamy_haze[m.dark]);
  shdr_1f(cur, "u_trans", m.transmission);
  shdr_1f(cur, "u_shine", m.shine);
  shdr_1f(cur, "u_wind", m.wind);
  shdr_1f(cur, "u_alpha", m.alpha);
  shdr_bind(cur);

  return cur;
}

void ani_mod_draw(ani_mod *m, anime *a, draw_src s, cam *c, m4 t, int id) {
  for (int i = 0; i < m->n_meshes; i++) {
    shdr *sh = ani_mod_get_sh(s, c, a, m->meshes[i].mat, t);
    shdr_1i(sh, "u_id", id);
    (m->meshes[i].mat.cull ? gl_enable : gl_disable)(GL_CULL_FACE);

    vao_bind(&m->meshes[i].vao);
    gl_draw_elements(GL_TRIANGLES, m->meshes[i].n_inds, GL_UNSIGNED_INT, 0);

    $.n_tris += m->meshes[i].n_inds / 3;
  }

  gl_disable(GL_CULL_FACE);
}

m4 dummy[100];
anime ani_dummy = {
  .final_mats = dummy
};