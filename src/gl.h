#pragma once

#include "lib/glad/glad.h"
#include <intrin.h>
#include "typedefs.h"
#include "err.h"
#include "body.h"
#include "map.h"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// forward declaration
struct app;

typedef enum draw_src {
  ds_cam,
  ds_shade,
  ds_n,
} draw_src;

typedef struct plane {
  v3 norm;
  float dist;
} plane;

plane plane_new(v3 point, v3 norm);

float plane_sdf(plane p, v3 pt);

typedef struct frustum {
  plane top, bottom, left, right, far, near;
} frustum;

typedef struct cam {
  v3 pos, front, up, right, world_up;
  float target_yaw, yaw, target_pitch, pitch, zoom, aspect, ortho_size, dist;

  v2 last_mouse_pos;
  bool has_last, shade;
  m4 vp, cvp;

  frustum frustum_shade, frustum_cam;
} cam;

cam cam_new(v3 pos, v3 world_up, float yaw, float pitch, float aspect);

void cam_tick(cam *c);

void cam_rot(cam *c);

v3 cam_get_eye(cam *c);

m4 cam_get_look(cam *c);

m4 cam_get_proj(cam *c);

int cam_test_box(cam *c, box3 b, draw_src s);

typedef struct shdr {
  u32 id;

  map locs;
} shdr;

typedef struct shdr_s {
  u32 type;
  char const *path;
} shdr_s;

int shdr_get_loc(shdr *s, char const *n);

shdr shdr_new(u32 n, shdr_s *shdrs);

void shdr_bind(shdr *s);

void shdr_m4f(shdr *s, char const *n, m4 m);

void shdr_1i(shdr *s, char const *n, int m);

void shdr_1f(shdr *s, char const *n, float m);

void shdr_2f(shdr *s, char const *n, v2 m);

void shdr_3f(shdr *s, char const *n, v3 m);

void shdr_3fv(shdr *s, char const *n, v3 *m, int amt);

void shdr_4f(shdr *s, char const *n, v4 m);

typedef struct buf {
  u32 id;
  u32 type;
} buf;

buf buf_new(u32 type);

buf *buf_heap_new(u32 type);

void *buf_rw(buf *b, size_t size);

void
buf_data_n(buf *b, u32 usage, ssize_t elem_size, ssize_t n, void *data);

void buf_data(buf *b, u32 usage, ssize_t size_in_bytes, void *data);

void buf_bind(buf *b);

typedef struct attrib {
  int size;
  u32 type;
} attrib;

static attrib attr_1f = {1, GL_FLOAT};
static attrib attr_2f = {2, GL_FLOAT};
static attrib attr_3f = {3, GL_FLOAT};
static attrib attr_4f = {4, GL_FLOAT};

static attrib attr_1i = {1, GL_INT};
static attrib attr_2i = {2, GL_INT};
static attrib attr_3i = {3, GL_INT};
static attrib attr_4i = {4, GL_INT};

typedef struct vao {
  u32 id;
  attrib *attrs;
  int n_attrs;
  int stride;
} vao;

void vao_bind(vao *v);

vao
vao_new(buf *vbo, buf *ibo, u32 n, attrib *attrs);

void imod_opti_vao(vao *v, buf *model, buf *id);

int attrib_get_size_in_bytes(attrib *attr);

typedef struct tex_spec {
  int width, height, min_filter, mag_filter;
  u32 internal_format, format;
  bool multisample, shadow;

  // owning!
  // can be null!
  u8 *pixels;
} tex_spec;

tex_spec tex_spec_invalid();

tex_spec tex_spec_rgba8(int width, int height, int filter);
tex_spec tex_spec_r32i(int width, int height, int filter);

tex_spec tex_spec_rgba16(int width, int height, int filter);

tex_spec tex_spec_rgba16_msaa(int width, int height, int filter);

tex_spec tex_spec_r16(int width, int height, int filter);

tex_spec tex_spec_r8v(int width, int height, int filter, u8 *pixels);

tex_spec tex_spec_depth32(int width, int height, int filter);

tex_spec tex_spec_shadow(int width, int height, int filter);

typedef struct tex {
  u32 id;
  tex_spec spec;
} tex;

tex tex_new(tex_spec spec);

void tex_resize(tex *t, int width, int height);

void tex_bind(tex *t, u32 unit);

void tex_del(tex *t);

typedef struct fbo_spec {
  u32 id;
  tex_spec spec;
} fbo_spec;

typedef struct fbo_buf {
  u32 id;
  tex tex;
} fbo_buf;

typedef struct fbo {
  u32 id;
  fbo_buf *bufs;
  u32 n_bufs;
} fbo;

fbo fbo_new(u32 n, fbo_spec *spec);

void fbo_bind(fbo *f);

void fbo_draw_bufs(fbo *f, int n, u32 *bufs);

void fbo_read_buf(fbo *f, u32 buf);

bool is_gl_buf_color_attachment(u32 it);

tex *fbo_tex_at(fbo *f, u32 buf);

void fbo_blit(fbo *src, fbo *dst, u32 src_a, u32 dst_a,
              u32 filter);

void fbo_resize(fbo *f, int width, int height, u32 n, u32 *bufs);

typedef struct to_cmyk {
  // non owning!
  tex *tex;
  int unit;
} to_cmyk;

void to_cmyk_up(shdr *s, to_cmyk args);

typedef struct halftone {
  // non owning!
  tex *cmyk;
  int unit;

  v2 scr_size;
  int dots_per_line;
} halftone;

void halftone_up(shdr *s, halftone args);

typedef struct blit {
  // non owning!
  tex *tex;
  int unit;
} blit;

void blit_up(shdr *s, blit args);

typedef struct dof {
  // non owning!
  tex *tex;
  int tex_unit;

  tex *depth;
  int depth_unit;

  int size;
  float separation, min_depth, max_depth;
  v2 screen_size;
} dof;

void dof_up(shdr *s, dof args);

typedef struct crt {
  // non owning!
  tex *tex;
  int unit;

  float aspect;
  float lores;
} crt;

void crt_up(shdr *s, crt args);

typedef struct blur {
  // non owning!
  tex *tex;
  int unit;

  v2 scr_size;
} blur;

void blur_up(shdr *s, blur args);

typedef struct dither {
  // non owning!
  tex *tex;
  int unit;

  v3 *pal;
  int pal_size;
} dither;

void dither_up(shdr *s, dither args);

typedef struct obj_vtx {
  v3 pos;
  v3 norm;
} obj_vtx;

typedef struct mtl {
  u32 light, dark;
  v3 light_model; // ambient, diffuse, specular
  float shine;
  int cull;
  float wind;
  float transmission;
  int line;
  float alpha;
} mtl;

typedef struct mesh {
  obj_vtx *vtxs;
  int n_vtxs;
  int n_inds;
  mtl mat;

  buf vbo, ibo;
  vao vao;
  char const *name;
} mesh;

void vao_del(struct vao *v);
void buf_del(buf *b);

typedef struct mod {
  tex *texes;
  int n_texes;

  mesh *meshes;
  int n_meshes;

  box3 bounds;
} mod;

map mod_load_mtl(char const *path);

shdr *mod_get_sh(draw_src s, cam *c, mtl m, m4 t);

mod mod_new(char const *path);

mod mod_new_mem(const char *mem, size_t len, const char *path);

void mod_draw(mod *m, draw_src s, cam *c, m4 t, int id);

typedef struct imod {
  tex *texes;
  int n_texes;

  mesh *meshes;
  int n_meshes;

  m4 *model;
  int *id;
  buf model_buf;
  buf id_buf;

  box3 bounds;
} imod;

imod *imod_new(mod m);
shdr *imod_get_sh(draw_src s, cam *c, mtl m);
void imod_draw(draw_src s, cam *c);
void imod_add(imod *m, m4 t, int id);

mod mod_new_indirect_mtl(const char *path, const char *mtl);

int *quad_indices(int w, int h);