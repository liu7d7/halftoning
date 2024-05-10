#pragma once

#include "lib/glad/glad.h"
#include <intrin.h>
#include "typedefs.h"
#include "err.h"
#include "body.h"
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
  v3f norm;
  float dist;
} plane;

plane plane_new(v3f point, v3f norm);

float plane_sdf(plane p, v3f pt);

typedef struct frustum {
  plane top, bottom, left, right, far, near;
} frustum;

typedef struct cam {
  v3f pos, front, up, right, world_up;
  float target_yaw, yaw, target_pitch, pitch, zoom, aspect, ortho_size, dist;

  v2f last_mouse_pos;
  bool has_last, shade;
  m4f vp, cvp;

  frustum frustum_shade, frustum_cam;
} cam;

cam cam_new(v3f pos, v3f world_up, float yaw, float pitch, float aspect);

void cam_tick(cam *c, struct app *a);

void cam_rot(cam *c);

v3f cam_get_eye(cam *c);

m4f cam_get_look(cam *c);

m4f cam_get_proj(cam *c);

int cam_test_box(cam *c, box3 b, draw_src s);

typedef struct shdr {
  uint id;
} shdr;

typedef struct shdr_spec {
  uint type;
  char const *path;
} shdr_spec;

shdr shdr_new(uint n, shdr_spec *shdrs);

void shdr_bind(shdr *s);

void shdr_m4f(shdr *s, char const *n, m4f m);

void shdr_1i(shdr *s, char const *n, int m);

void shdr_1f(shdr *s, char const *n, float m);

void shdr_2f(shdr *s, char const *n, v2f m);

void shdr_3f(shdr *s, char const *n, v3f m);

void shdr_3fv(shdr *s, char const *n, v3f *m, int amt);

void shdr_4f(shdr *s, char const *n, v4f m);

typedef struct buf {
  uint id;
  uint type;
} buf;

buf buf_new(uint type);

buf *buf_heap_new(uint type);

void *buf_rw(buf *b, size_t size);

void
buf_data_n(buf *b, uint usage, ssize_t elem_size, ssize_t n, void *data);

void buf_data(buf *b, uint usage, ssize_t size_in_bytes, void *data);

void buf_bind(buf *b);

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

typedef struct vao {
  uint id;
  attrib *attrs;
  int n_attrs;
  int stride;
} vao;

void vao_bind(vao *v);

vao
vao_new(buf *vbo, buf *ibo, uint n, attrib *attrs);

void imod_opti_vao(vao *v, buf *model);

int attrib_get_size_in_bytes(attrib *attr);

typedef struct tex_spec {
  int width, height, min_filter, mag_filter;
  uint internal_format, format;
  bool multisample, shadow;

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

tex_spec tex_spec_depth32(int width, int height, int filter);

tex_spec tex_spec_shadow(int width, int height, int filter);

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

void to_cmyk_up(shdr *s, to_cmyk args);

typedef struct halftone {
  // non owning!
  tex *cmyk;
  int unit;

  v2f scr_size;
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
  v2f screen_size;
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

  v2f scr_size;
} blur;

void blur_up(shdr *s, blur args);

typedef struct dither {
  // non owning!
  tex *tex;
  int unit;

  v3f *pal;
  int pal_size;
} dither;

void dither_up(shdr *s, dither args);

#define mod_max_bone_influence 4

typedef struct obj_vtx {
  v3f pos;
  v3f norm;
} obj_vtx;

typedef struct mtl {
  uint light, dark;
  v3f light_model; // ambient, diffuse, specular
  float shine;
  int cull;
  float wind;
  float transmission;
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

shdr *mod_get_sh(draw_src s, cam *c, mtl m, m4f t);

mod mod_new(char const *path);

mod mod_new_mem(const char *mem, size_t len, const char *path);

void mod_draw(mod *m, draw_src s, cam *c, m4f t);

typedef struct imod {
  tex *texes;
  int n_texes;

  mesh *meshes;
  int n_meshes;

  m4f *model;
  buf model_buf;

  box3 bounds;
} imod;

imod *imod_new(mod m);
shdr *imod_get_sh(draw_src s, cam *c, mtl m);
void imod_draw(draw_src s, cam *c);
void imod_add(imod *m, m4f trans);

mod mod_new_indirect_mtl(const char *path, const char *mtl);

int *quad_indices(int w, int h);