#pragma once

#include "lib/glad/glad.h"
#include <intrin.h>
#include "typedefs.h"
#include "err.h"
#include <cglm/cglm.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// forward declaration
struct game;

struct cam {
  vec3 pos, front, up, right, world_up;
  float target_yaw, yaw, target_pitch, pitch, speed, vel, sens, zoom;

  vec2 last_mouse_pos;
  bool has_last;
};

struct cam cam(vec3 pos, vec3 world_up, float yaw, float pitch);

void cam_update(struct cam* c, struct game* g);

void cam_look(struct cam* c, mat4 res);

void cam_proj(struct cam* c, float const* win_size, mat4 res);

struct shader {
  uint id;
};

struct shader_spec {
  uint type;
  char const* path;
};

struct shader shader(uint n, struct shader_spec* shaders);

void shader_bind(struct shader* s);

void shader_mat4(struct shader* s, char const* n, mat4 m);

void shader_int(struct shader* s, char const* n, int m);

void shader_float(struct shader* s, char const* n, float m);

void shader_vec2(struct shader* s, char const* n, vec2 m);

void shader_vec3(struct shader* s, char const* n, vec3 m);

void shader_vec4(struct shader* s, char const* n, vec4 m);

struct buf {
  uint id;
  uint type;
};

struct buf buf(uint type);

struct buf* buf_heap_new(uint type);

void
buf_data_n(struct buf* b, uint usage, ssize_t elem_size, ssize_t n, void* data);

void buf_data(struct buf* b, uint usage, ssize_t size_in_bytes, void* data);

void buf_bind(struct buf* b);

struct vao {
  uint id;
};

void vao_bind(struct vao* v);

struct attrib {
  int size;
  uint type;
};

static struct attrib attr_float_1 = {1, GL_FLOAT};
static struct attrib attr_float_2 = {2, GL_FLOAT};
static struct attrib attr_float_3 = {3, GL_FLOAT};
static struct attrib attr_float_4 = {4, GL_FLOAT};

static struct attrib attr_int_1 = {1, GL_INT};
static struct attrib attr_int_2 = {2, GL_INT};
static struct attrib attr_int_3 = {3, GL_INT};
static struct attrib attr_int_4 = {4, GL_INT};

struct vao
vao(struct buf* vbo, struct buf* ibo, uint n, struct attrib* attrs);

int attrib_get_size_in_bytes(struct attrib* attr);

struct tex_spec {
  int width, height, min_filter, mag_filter;
  uint internal_format, format;

  // owning!
  // can be null!
  byte* pixels;
};

struct tex_spec tex_spec_invalid();

struct tex_spec tex_spec_rgba8(int width, int height, int filter);

struct tex_spec tex_spec_rgba16(int width, int height, int filter);

struct tex_spec tex_spec_r16(int width, int height, int filter);

struct tex_spec tex_spec_depth24(int width, int height, int filter);

struct tex {
  uint id;
  struct tex_spec spec;
};

struct tex tex(struct tex_spec spec);

void tex_resize(struct tex* t, int width, int height);

void tex_bind(struct tex* t, uint unit);

void tex_del(struct tex* t);

struct fbo_spec {
  uint id;
  struct tex_spec spec;
};

struct fbo_buf {
  uint id;
  struct tex tex;
};

struct fbo {
  uint id;
  struct fbo_buf* bufs;
  uint n_bufs;
};

struct fbo fbo(uint n, struct fbo_spec* spec);

void fbo_bind(struct fbo* f);

void fbo_draw_bufs(struct fbo* f, int n, uint* bufs);

void fbo_read_buf(struct fbo* f, uint buf);

bool is_gl_buf_color_attachment(uint it);

struct tex* fbo_tex_at(struct fbo* f, uint buf);

void fbo_blit(struct fbo* src, struct fbo* dst, uint src_a, uint dst_a,
              uint filter);

void fbo_resize(struct fbo* f, int width, int height, uint n, uint* bufs);

struct to_cmyk {
  // non owning!
  struct tex* tex;
  int unit;
};

void to_cmyk_up(struct shader* s, struct to_cmyk args);

struct halftone {
  // non owning!
  struct tex* cmyk;
  int unit;

  vec2 scr_size;
  int dots_per_line;
};

void halftone_up(struct shader* s, struct halftone args);

struct blit {
  // non owning!
  struct tex* tex;
  int unit;
};

void blit_up(struct shader* s, struct blit args);

struct blur {
  // non owning!
  struct tex* tex;
  int unit;

  vec2 scr_size;
};

void blur_up(struct shader* s, struct blur args);

#define mod_max_bone_influence 4

struct mod_vtx {
  vec3 pos;
  vec3 norm;
  vec2 uvs;
};

struct mod_vtx mod_vtx(vec3 pos, vec3 norm, vec2 uvs);

struct mesh {
  struct mod_vtx* vtxs;
  int n_vtxs;
  int n_inds;

  struct vao vao;
};

struct mod {
  struct tex* texes;
  int n_texes;

  struct mesh* meshes;
  int n_meshes;

  struct shader shader;
};

struct mesh
mod_load_mesh(struct mod* m, struct aiMesh* mesh, struct aiScene const* scene);

void
mod_load(struct mod* m, struct aiNode* node, struct aiScene const* scene);

struct mod mod(char const* path);

void mod_draw(struct mod* m, struct game* g);