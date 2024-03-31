#pragma once

#include "lib/glad/glad.h"
#include <intrin.h>
#include "typedefs.h"
#include "err.h"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// forward declaration
struct app;

typedef struct cam {
  v3f pos, front, up, right, world_up;
  v3f prev_pos;
  float prev_yaw, yaw, prev_pitch, pitch, speed, vel, sens, zoom, aspect;

  v2f last_mouse_pos;
  bool has_last;
} cam;

cam cam_new(v3f pos, v3f world_up, float yaw, float pitch, float aspect);

void cam_tick(cam *c, struct app *g);

void cam_rot(cam *c, float d);

v3f cam_get_pos(cam *c, float d);

float cam_get_yaw(cam *c, float d);

float cam_get_pitch(cam *c, float d);

m4f cam_get_look(cam *c, float d);

m4f cam_get_proj(cam *c);

typedef struct shader {
  uint id;
} shader;

typedef struct shader_spec {
  uint type;
  char const *path;
} shader_spec;

shader shader_new(uint n, shader_spec *shaders);

void shader_bind(shader *s);

void shader_mat4(shader *s, char const *n, m4f m);

void shader_int(shader *s, char const *n, int m);

void shader_float(shader *s, char const *n, float m);

void shader_vec2(shader *s, char const *n, v2f m);

void shader_vec3(shader *s, char const *n, v3f m);

void shader_vec4(shader *s, char const *n, v4f m);

typedef struct buf {
  uint id;
  uint type;
} buf;

buf buf_new(uint type);

buf *buf_heap_new(uint type);

void
buf_data_n(buf *b, uint usage, ssize_t elem_size, ssize_t n, void *data);

void buf_data(buf *b, uint usage, ssize_t size_in_bytes, void *data);

void buf_bind(buf *b);

typedef struct vao {
  uint id;
} vao;

void vao_bind(vao *v);

typedef struct attrib {
  int size;
  uint type;
} attrib;

static attrib attr_1f = {1, GL_FLOAT};
static attrib attr_2f = {2, GL_FLOAT};
static attrib attr_3f = {3, GL_FLOAT};
static attrib attr_4f = {4, GL_FLOAT};

static attrib attr_1i = {1, GL_INT};
static attrib attr_2i = {2, GL_INT};
static attrib attr_3i = {3, GL_INT};
static attrib attr_4i = {4, GL_INT};

vao
vao_new(buf *vbo, buf *ibo, uint n, attrib *attrs);

int attrib_get_size_in_bytes(attrib *attr);

typedef struct tex_spec {
  int width, height, min_filter, mag_filter;
  uint internal_format, format;
  bool multisample;

  // owning!
  // can be null!
  u8 *pixels;
} tex_spec;

tex_spec tex_spec_invalid();

tex_spec tex_spec_rgba8(int width, int height, int filter);

tex_spec tex_spec_rgba16(int width, int height, int filter);

tex_spec tex_spec_rgba16_msaa(int width, int height, int filter);

tex_spec tex_spec_r16(int width, int height, int filter);

tex_spec tex_spec_r8v(int width, int height, int filter, u8 *pixels);

tex_spec tex_spec_depth24(int width, int height, int filter);

typedef struct tex {
  uint id;
  tex_spec spec;
} tex;

tex tex_new(tex_spec spec);

void tex_resize(tex *t, int width, int height);

void tex_bind(tex *t, uint unit);

void tex_del(tex *t);

typedef struct fbo_spec {
  uint id;
  tex_spec spec;
} fbo_spec;

typedef struct fbo_buf {
  uint id;
  tex tex;
} fbo_buf;

typedef struct fbo {
  uint id;
  fbo_buf *bufs;
  uint n_bufs;
} fbo;

fbo fbo_new(uint n, fbo_spec *spec);

void fbo_bind(fbo *f);

void fbo_draw_bufs(fbo *f, int n, uint *bufs);

void fbo_read_buf(fbo *f, uint buf);

bool is_gl_buf_color_attachment(uint it);

tex *fbo_tex_at(fbo *f, uint buf);

void fbo_blit(fbo *src, fbo *dst, uint src_a, uint dst_a,
              uint filter);

void fbo_resize(fbo *f, int width, int height, uint n, uint *bufs);

typedef struct to_cmyk {
  // non owning!
  tex *tex;
  int unit;
} to_cmyk;

void to_cmyk_up(shader *s, to_cmyk args);

typedef struct halftone {
  // non owning!
  tex *cmyk;
  int unit;

  v2f scr_size;
  int dots_per_line;
} halftone;

void halftone_up(shader *s, halftone args);

typedef struct blit {
  // non owning!
  tex *tex;
  int unit;
} blit;

void blit_up(shader *s, blit args);

typedef struct blur {
  // non owning!
  tex *tex;
  int unit;

  v2f scr_size;
} blur;

void blur_up(shader *s, blur args);

#define mod_max_bone_influence 4

typedef struct mod_vtx {
  v3f pos;
  v3f norm;
  v2f uvs;
} mod_vtx;

typedef struct mesh {
  mod_vtx *vtxs;
  int n_vtxs;
  int n_inds;

  vao vao;
} mesh;

typedef struct mod {
  tex *texes;
  int n_texes;

  mesh *meshes;
  int n_meshes;
} mod;

shader *mod_get_shader(cam *c, m4f t, float d);

mesh mod_load_mesh(mod *m, struct aiMesh *mesh, struct aiScene const *scene);

void mod_load(mod *m, struct aiNode *node, struct aiScene const *scene);

mod mod_new(char const *path);

void mod_draw(mod *m, cam *c, m4f t, float d);

int *quad_indices(int w, int h);