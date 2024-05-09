#include "gl.h"
#include "err.h"
#include "app.h"
#include "arr.h"
#include "pal.h"

cam
cam_new(v3f pos, v3f world_up, float yaw, float pitch, float aspect) {
  const float default_zoom = 45.0f;

  cam c = {
    .front = {0, 0, -1},
    .yaw = yaw, .pitch = pitch, .target_pitch = pitch, .target_yaw = yaw,
    .zoom = default_zoom, .dist = 10,
    .has_last = 0,
    .aspect = aspect,
    .pos = pos,
    .world_up = world_up,
    .up = world_up
  };

  return c;
}

void cam_tick(cam *c, app *a) {
  if (c->has_last && a->is_mouse_captured) {
    v2f delta = v2_sub(a->mouse, c->last_mouse_pos);
    c->target_yaw += delta.x;
    c->target_pitch -= delta.y;

    if (c->target_pitch > 89.9f) c->target_pitch = 89.9f;
    if (c->target_pitch < -89.9f) c->target_pitch = -89.9f;
  }

  if (a->is_mouse_captured) {
    c->has_last = true;
    c->last_mouse_pos = a->mouse;
  }
}

m4f cam_get_look(cam *c) {
  return m4_look(cam_get_eye(c), c->front, c->up);
}

float const cam_near = 0.01f, cam_far =
  1.41421356f * 0.5f * chunk_size * world_draw_dist;

m4f cam_get_proj(cam *c) {
  if (c->shade) {
    return m4_ortho(-c->ortho_size / 2.f * c->aspect,
                    c->ortho_size / 2.f * c->aspect, c->ortho_size / 2.f,
                    -c->ortho_size / 2.f, -256.f,
                    256.f);
  } else {
    return m4_persp(rad(c->zoom), c->aspect, cam_near, cam_far);
  }
}

void shdr_verify(uint gl_id) {
  int is_ok;
  char info_log[1024];
  gl_get_shaderiv(gl_id, GL_COMPILE_STATUS, &is_ok);
  if (!is_ok) {
    gl_get_shader_info_log(gl_id, 1024, NULL, info_log);
    throw_c(info_log);
  }
}

uint shdr_compile(shdr_spec s) {
  FILE *f = fopen(s.path, "r");
  if (!f) {
    throw_c("Failed to open file for shdr_component!");
  }

  char *src = calloc(1 << 20, 1);
  char block[512];
  for (int i = 0; i < 512; i++) {
    block[i] = 0;
  }

  while (fgets(block, sizeof(block), f)) {
    if (block[0] == '#') {
      if (strncmp(block + 1, "include", 7) != 0) {
        goto not_import;
      }

      for (int i = 0; i < sizeof(block); i++) {
        if (block[i] == '>') {
          block[i] = '\0';
          break;
        }
      }

      char const *to_import = block + 1 + 7 + 2;
      char *file_contents = read_txt_file(to_import);
      strcat(src, file_contents);
      free(file_contents);
      continue;
    }

    not_import:;
    strcat(src, block);
  }

  uint gl_id = gl_create_shader(s.type);
  gl_shader_source(gl_id, 1, (char const *[]){src},
                   (int[]){(int)strlen(src)});
  gl_compile_shader(gl_id);
  shdr_verify(gl_id);

  free(src);
  fclose(f);

  return gl_id;
}

void prog_verify(uint gl_id) {
  int is_ok;
  char info_log[1024];
  gl_get_programiv(gl_id, GL_LINK_STATUS, &is_ok);
  if (!is_ok) {
    gl_get_program_info_log(gl_id, 1024, NULL, info_log);
    throw_c(info_log);
  }
}

shdr shdr_new(uint n, shdr_spec *shdrs) {
  uint gl_ids[n], gl_id = gl_create_program();

  for (int i = 0; i < n; i++) {
    gl_ids[i] = shdr_compile(shdrs[i]);
    gl_attach_shader(gl_id, gl_ids[i]);
  }

  gl_link_program(gl_id);
  prog_verify(gl_id);

  return (shdr){.id = gl_id};
}

struct vao vao_new(buf *vbo, buf *ibo, uint n, attrib *attrs) {
  int stride = 0;
  for (int i = 0; i < n; i++) {
    attrib a = attrs[i];
    stride += a.size * (int)(a.type == GL_INT ? sizeof(int) : sizeof(float));
  }

  struct vao v = {.id = 0, .n_attrs = n, .attrs = malloc(
    sizeof(attrib) * n), .stride = stride};
  memcpy(v.attrs, attrs, sizeof(attrib) * n);
  gl_create_vertex_arrays(1, &v.id);

  gl_vertex_array_vertex_buffer(v.id, 0, vbo->id, 0, stride);

  int offset = 0;
  for (int i = 0; i < n; i++) {
    attrib a = attrs[i];
    gl_enable_vertex_array_attrib(v.id, i);
    if (a.type == GL_FLOAT) {
      gl_vertex_array_attrib_format(v.id, i, a.size, GL_FLOAT, GL_FALSE,
                                    offset);
    } else {
      gl_vertex_array_attrib_i_format(v.id, i, a.size, GL_INT, offset);
    }

    gl_vertex_array_attrib_binding(v.id, i, 0);
    offset += attrib_get_size_in_bytes(&a);
  }

  if (ibo) gl_vertex_array_element_buffer(v.id, ibo->id);

  return v;
}

buf buf_new(uint type) {
  buf b = {.id = 0, .type = type};
  gl_create_buffers(1, &b.id);

  return b;
}

void *buf_rw(buf *b, size_t size) {
  gl_named_buffer_storage(b->id, size, NULL, GL_DYNAMIC_STORAGE_BIT);
  return gl_map_named_buffer(b->id, GL_READ_WRITE);
}

void buf_data_n(buf *b, uint usage, ssize_t elem_size, ssize_t n,
                void *data) {
  buf_data(b, usage, n * elem_size, data);
}

void buf_data(buf *b, uint usage, ssize_t size_in_bytes, void *data) {
  gl_named_buffer_data(b->id, size_in_bytes, data, usage);
}

void shdr_bind(shdr *s) {
  gl_use_program(s->id);
}

void vao_bind(struct vao *v) {
  gl_bind_vertex_array(v->id);
}

void buf_bind(buf *b) {
  gl_bind_buffer(b->type, b->id);
}

void shdr_m4f(shdr *s, char const *n, m4f m) {
  shdr_bind(s);
  gl_uniform_matrix_4fv(gl_get_uniform_location(s->id, n), 1, GL_TRUE,
                        &m.v[0][0]);
}

void shdr_1i(shdr *s, char const *n, int m) {
  shdr_bind(s);
  gl_uniform_1i(gl_get_uniform_location(s->id, n), m);
}

void shdr_1f(shdr *s, char const *n, float m) {
  shdr_bind(s);
  gl_uniform_1f(gl_get_uniform_location(s->id, n), m);
}

void shdr_2f(shdr *s, char const *n, v2f m) {
  shdr_bind(s);
  gl_uniform_2f(gl_get_uniform_location(s->id, n), m.x, m.y);
}

void shdr_3f(shdr *s, char const *n, v3f m) {
  shdr_bind(s);
  gl_uniform_3f(gl_get_uniform_location(s->id, n), m.x, m.y, m.z);
}

void shdr_3fv(shdr *s, char const *n, v3f *m, int amt) {
  shdr_bind(s);
  gl_uniform_3fv(gl_get_uniform_location(s->id, n), amt, (float *)m);
}

void shdr_4f(shdr *s, char const *n, v4f m) {
  shdr_bind(s);
  gl_uniform_4f(gl_get_uniform_location(s->id, n), m.x, m.y, m.z, m.w);
}

int attrib_get_size_in_bytes(attrib *attr) {
  return attr->size * (int)(attr->type == GL_INT ? sizeof(int) : sizeof(float));
}

tex_spec tex_spec_invalid() {
  return (tex_spec){};
}

tex_spec tex_spec_rgba8(int width, int height, int filter) {
  return (tex_spec){
    .width = width, .height = height, .min_filter = filter, .mag_filter = filter,
    .internal_format = GL_RGBA8, .format = GL_RGBA, .pixels = NULL, .multisample = false
  };
}

tex_spec tex_spec_rgba16(int width, int height, int filter) {
  return (tex_spec){
    .width = width, .height = height, .min_filter = filter, .mag_filter = filter,
    .internal_format = GL_RGBA16F, .format = GL_RGBA, .pixels = NULL, .multisample = false
  };
}

tex_spec tex_spec_rgba16_msaa(int width, int height, int filter) {
  return (tex_spec){
    .width = width, .height = height, .min_filter = filter, .mag_filter = filter,
    .internal_format = GL_RGBA16F, .format = GL_RGBA, .pixels = NULL, .multisample = true
  };
}

tex_spec tex_spec_r16(int width, int height, int filter) {
  return (tex_spec){
    .width = width, .height = height, .min_filter = filter, .mag_filter = filter,
    .internal_format = GL_R16F, .format = GL_RED, .pixels = NULL, .multisample = false
  };
}

tex_spec tex_spec_r8v(int width, int height, int filter, u8 *pixels) {
  return (tex_spec){
    .width = width, .height = height, .min_filter = filter, .mag_filter = filter,
    .internal_format = GL_R8, .format = GL_RED, .pixels = pixels, .multisample = false
  };
}

tex_spec tex_spec_depth32(int width, int height, int filter) {
  return (tex_spec){
    .width = width, .height = height, .min_filter = filter, .mag_filter = filter,
    .internal_format = GL_DEPTH_COMPONENT32, .format = GL_DEPTH_COMPONENT, .pixels = NULL, .multisample = false
  };
}

tex tex_new(tex_spec spec) {
  tex t = {.id = 0, .spec = spec};

  if (spec.multisample) {
    gl_create_textures(GL_TEXTURE_2D_MULTISAMPLE, 1, &t.id);
    gl_texture_storage_2d_multisample(t.id, 4, spec.internal_format, spec.width,
                                      spec.height, true);
  } else {
    gl_create_textures(GL_TEXTURE_2D, 1, &t.id);
    if (spec.format == GL_DEPTH_COMPONENT) {
      gl_texture_parameteri(t.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      gl_texture_parameteri(t.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    } else {
      gl_texture_parameteri(t.id, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
      gl_texture_parameteri(t.id, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    }
    
    if (spec.shadow) {
      gl_texture_parameteri(t.id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
      gl_texture_parameteri(t.id, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
    }

    gl_texture_parameteri(t.id, GL_TEXTURE_MIN_FILTER, spec.min_filter);
    gl_texture_parameteri(t.id, GL_TEXTURE_MAG_FILTER, spec.mag_filter);
    gl_texture_storage_2d(t.id, 1, spec.internal_format, spec.width,
                          spec.height);

    if (spec.pixels) {
      gl_texture_sub_image_2d(t.id, 0, 0, 0, spec.width, spec.height,
                              spec.format,
                              GL_UNSIGNED_BYTE, spec.pixels);
    }
  }

  return t;
}

void tex_resize(tex *t, int width, int height) {
  if (t->spec.pixels) {
    throw_c("Can't resize a texture that specifies its pixels!");
  }

  tex_spec resized_spec = t->spec;
  resized_spec.width = width, resized_spec.height = height;
  tex resized = tex_new(resized_spec);

  gl_delete_textures(1, &t->id);
  *t = resized;
}

void tex_bind(tex *t, uint unit) {
  if (unit >= 16) throw_c("Unit too high!");
  gl_active_texture(unit + GL_TEXTURE0);
  gl_bind_texture(GL_TEXTURE_2D, t->id);
}

void tex_del(tex *t) {
  gl_delete_textures(1, &t->id);
  if (t->spec.pixels) {
    free(t->spec.pixels);
    t->spec.pixels = NULL;
  }
}

fbo fbo_new(uint n, fbo_spec *spec) {
  fbo f = {.id = 0, .bufs = malloc(
    n * sizeof(fbo_buf)), .n_bufs = n};
  gl_create_framebuffers(1, &f.id);
  for (int i = 0; i < n; i++) {
    fbo_spec s = spec[i];
    tex t = tex_new(s.spec);
    f.bufs[i] = (fbo_buf){.id = s.id, .tex = t};
    gl_named_framebuffer_texture(f.id, s.id, t.id, 0);
  }

  return f;
}

void fbo_bind(fbo *f) {
  gl_bind_framebuffer(GL_FRAMEBUFFER, f->id);
}

void fbo_draw_bufs(fbo *f, int n, uint *bufs) {
  gl_named_framebuffer_draw_buffers(f->id, n, bufs);
}

void fbo_read_buf(fbo *f, uint buf) {
  gl_named_framebuffer_read_buffer(f->id, buf);
}

bool is_gl_buf_color_attachment(uint it) {
  return it >= GL_COLOR_ATTACHMENT0 && it <= GL_COLOR_ATTACHMENT31;
}

tex *fbo_tex_at(fbo *f, uint buf) {
  for (int i = 0; i < f->n_bufs; i++) {
    if (f->bufs[i].id == buf) {
      return &f->bufs[i].tex;
    }
  }

  throw_c("Failed to find attachment of framebuffer!");
}

void
fbo_blit(fbo *src, fbo *dst, uint src_a, uint dst_a,
         uint filter) {
  // @formatter:off
  int src_mask =
    is_gl_buf_color_attachment(src_a) ?
      GL_COLOR_BUFFER_BIT
    : src_a == GL_DEPTH_ATTACHMENT ?
      GL_DEPTH_BUFFER_BIT
    : (throw_c("Failed to parse out a src_mask!"), -1);

  int dst_mask =
    is_gl_buf_color_attachment(dst_a) ?
      GL_COLOR_BUFFER_BIT
    : dst_a == GL_DEPTH_ATTACHMENT ?
      GL_DEPTH_BUFFER_BIT
    : (throw_c("Failed to parse out a dst_mask!"), -1);
  // @formatter:on

  if (src_mask != dst_mask) throw_c("Src and dst masks do not match!");

  tex *src_tex = fbo_tex_at(src, src_a);
  tex *dst_tex = fbo_tex_at(dst, dst_a);

  if (src_mask == GL_COLOR_BUFFER_BIT) {
    fbo_read_buf(src, src_a);
    fbo_draw_bufs(dst, 1, (uint[]){dst_a});
  }

  gl_blit_named_framebuffer(
    src->id, dst->id,
    0, 0, src_tex->spec.width, src_tex->spec.height,
    0, 0, dst_tex->spec.width, dst_tex->spec.height,
    src_mask, filter);
}

void fbo_resize(fbo *f, int width, int height, uint n, uint *bufs) {
  for (int i = 0; i < n; i++) {
    tex *t = fbo_tex_at(f, bufs[i]);
    tex_resize(t, width, height);
    gl_named_framebuffer_texture(f->id, bufs[i], t->id, 0);
  }
}

void to_cmyk_up(shdr *s, to_cmyk args) {
  shdr_bind(s);
  tex_bind(args.tex, args.unit);
  shdr_1i(s, "u_tex", args.unit);
}

void halftone_up(shdr *s, halftone args) {
  shdr_bind(s);
  tex_bind(args.cmyk, args.unit);
  shdr_1i(s, "u_cmyk", args.unit);
  shdr_2f(s, "u_scr_size", args.scr_size);
  shdr_1f(s, "u_dots_per_line", (float)args.dots_per_line);
}

void blit_up(shdr *s, blit args) {
  shdr_bind(s);
  tex_bind(args.tex, args.unit);
  shdr_1i(s, "u_tex", args.unit);
}

void blur_up(shdr *s, blur args) {
  shdr_bind(s);
  tex_bind(args.tex, args.unit);
  shdr_1i(s, "u_tex", args.unit);
  shdr_2f(s, "u_scr_size", args.scr_size);
}

void dither_up(shdr *s, dither args) {
  shdr_bind(s);
  tex_bind(args.tex, args.unit);
  shdr_1i(s, "u_tex0", args.unit);
  shdr_3fv(s, "u_pal", args.pal, args.pal_size);
  shdr_1i(s, "u_pal_size", args.pal_size);
}

mesh
mod_load_mesh(mod *m, lmap *mats, box3 *b, struct aiMesh *mesh,
              const struct aiScene *scene) {
  obj_vtx *vtxs = malloc(sizeof(obj_vtx) * mesh->mNumVertices);

  char *name = mesh->mName.data;
  mtl *mat = lmap_at(mats, &name);
  if (!mat) {
    fprintf(stderr, "problem! %s in ", mesh->mName.data);
    throw_c("mod_load_mesh: failed to find material in map!");
  }

  for (int i = 0; i < mesh->mNumVertices; i++) {
    v3f pos = *(v3f *)&mesh->mVertices[i], norm = *(v3f *)&mesh->mNormals[i];

    vtxs[i] = (obj_vtx){pos, norm};
    b->min = v3_min(b->min, vtxs[i].pos);
    b->max = v3_max(b->max, vtxs[i].pos);
  }

  buf vbo = buf_new(GL_ARRAY_BUFFER), ibo = buf_new(GL_ELEMENT_ARRAY_BUFFER);

  buf_data_n(&vbo,
             GL_DYNAMIC_DRAW,
             sizeof(obj_vtx),
             mesh->mNumVertices,
             vtxs);

  uint *inds = arr_new(uint, 4);
  for (int i = 0; i < mesh->mNumFaces; i++) {
    for (int j = 0; j < mesh->mFaces[i].mNumIndices; j++) {
      arr_add(&inds, &mesh->mFaces[i].mIndices[j]);
    }
  }

  buf_data_n(&ibo,
             GL_DYNAMIC_DRAW,
             sizeof(uint),
             arr_len(inds),
             inds);

  struct mesh me = {
    .vtxs = vtxs,
    .n_vtxs = (int)mesh->mNumVertices,
    .n_inds = arr_len(inds),
    .vao = vao_new(&vbo, &ibo, 2,
                   (attrib[]){attr_3f, attr_3f}),
    .mat = *mat
  };

  arr_del(inds);

  return me;
}

void
mod_load(mod *m, lmap *mats, box3 *b, struct aiNode *node,
         const struct aiScene *scene) {
  for (int i = 0; i < node->mNumMeshes; i++) {
    struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    m->meshes[m->n_meshes++] = mod_load_mesh(m, mats, b, mesh, scene);
  }

  for (int i = 0; i < node->mNumChildren; i++) {
    mod_load(m, mats, b, node->mChildren[i], scene);
  }
}

lmap mod_load_mtl(char const *path) {
  ssize_t path_len = strlen(path);
  ssize_t last_dot_idx = path_len - 1;
  for (; last_dot_idx >= 0 && path[last_dot_idx] != '.'; last_dot_idx--) {}

  char mtl_path[384];
  memcpy(mtl_path, path, last_dot_idx);
  char const ext[] = ".mtl.txt";
  for (ssize_t i = last_dot_idx; i < last_dot_idx + sizeof(ext); i++) {
    mtl_path[i] = ext[i - last_dot_idx];
  }

  FILE *f = fopen(mtl_path, "r");
  if (!f) {
    fprintf(stderr, "problem! %s in ", mtl_path);
    throw_c("mod_load_mtl: failed to load material file!");
  }

  auto mats = lmap_new(4, sizeof(char *), sizeof(mtl), 0.75f, str_eq, str_hash);
  char line[256];
  char name[64];
  while (fgets(line, 256, f)) {
    mtl mat;
    int r = sscanf(line, "%63s %u %u %f %f %f %f %d %f", name, &mat.dark,
                   &mat.light, &mat.light_model.x, &mat.light_model.y,
                   &mat.light_model.z, &mat.shine, &mat.cull, &mat.wind);
    if (r < 9) {
      fprintf(stderr, "problem! %s in ", line);
      throw_c("mod_load_mtl: failed to parse mtl!");
    }
    size_t name_size = strlen(name) + 1;
    char *heap_name = malloc(name_size);
    strcpy_s(heap_name, name_size, name);
    lmap_add(&mats, &heap_name, &mat);
  }

  fclose(f);

  return mats;
}

mod mod_from_scene(struct aiScene const *scene, const char *path) {
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    throw_c(aiGetErrorString());
  }

  mod m = {
    .meshes = malloc(sizeof(mesh) * scene->mNumMeshes),
  };

  lmap mats = mod_load_mtl(path);

  float large = 1e20f;
  box3 b = box3_new((v3f){large, large, large}, (v3f){-large, -large, -large});

  mod_load(&m, &mats, &b, scene->mRootNode, scene);
  m.bounds = b;

  aiReleaseImport(scene);

  return m;
}

mod mod_new(const char *path) {
  struct aiScene const *scene =
    aiImportFile(path, aiProcessPreset_TargetRealtime_Fast);

  return mod_from_scene(scene, path);
}

mod mod_new_mem(const char *mem, size_t len, const char *path) {
  struct aiScene const *scene =
    aiImportFileFromMemory(mem, len, aiProcessPreset_TargetRealtime_Fast, "obj");

  return mod_from_scene(scene, path);
}

void mod_draw(mod *m, draw_src s, cam *c, m4f t) {
  for (int i = 0; i < m->n_meshes; i++) {
    mod_get_sh(s, c, m->meshes[i].mat, t);
    (m->meshes[i].mat.cull ? gl_enable : gl_disable)(GL_CULL_FACE);

    vao_bind(&m->meshes[i].vao);
    gl_draw_elements(GL_TRIANGLES, m->meshes[i].n_inds, GL_UNSIGNED_INT, 0);

    the_app.n_tris += m->meshes[i].n_inds / 3;
  }

  gl_disable(GL_CULL_FACE);
}

shdr *mod_get_sh(draw_src s, cam *c, mtl m, m4f t) {
  static shdr *cam = NULL;
  static shdr *shade = NULL;
  if (!cam) {
    cam = objdup(shdr_new(2,
                          (shdr_spec[]){
                            {GL_VERTEX_SHADER,   "res/mod.vsh"},
                            {GL_FRAGMENT_SHADER, "res/mod_light.fsh"},
                          }));

    shade = objdup(shdr_new(2,
                            (shdr_spec[]){
                              {GL_VERTEX_SHADER,   "res/mod_depth.vsh"},
                              {GL_FRAGMENT_SHADER, "res/mod_depth.fsh"},
                            }));
  }

  shdr *cur = s == ds_cam ? cam : shade;

  shdr_m4f(cur, "u_vp", c->vp);
  shdr_3f(cur, "u_eye", cam_get_eye(c));
  shdr_3f(cur, "u_light_model", m.light_model);
  shdr_3f(cur, "u_light", dreamy_haze[m.light]);
  shdr_3f(cur, "u_dark", dreamy_haze[m.dark]);
  shdr_1f(cur, "u_shine", m.shine);
  shdr_1f(cur, "u_wind", m.wind);
  shdr_m4f(cur, "u_model", t);
  shdr_1f(cur, "u_time", app_now() / 1000.f);
  shdr_bind(cur);

  return cur;
}

void cam_make_frustum(cam *c) {
  frustum *f = &c->frustum_shade;
  float half_v = cam_far * tanf(c->zoom * .5f);
  float half_h = half_v * c->aspect;
  v3f front = v3_mul(c->front, cam_far + 4.f);
  v3f eye = v3_sub(cam_get_eye(c), v3_mul(c->front, 4.f));

  f->near = plane_new(v3_add(eye, v3_mul(c->front, cam_near)), c->front);
  f->far = plane_new(v3_add(eye, front), v3_neg(c->front));
  f->right = plane_new(eye, v3_cross(v3_sub(front, v3_mul(c->right, half_h)), c->up));
  f->right = plane_new(eye, v3_cross(c->up, v3_add(front, v3_mul(c->right, half_h))));
  f->top = plane_new(eye, v3_cross(c->right, v3_sub(front, v3_mul(c->up, half_v))));
  f->bottom = plane_new(eye, v3_cross(v3_add(front, v3_mul(c->up, half_v)), c->right));

  f = &c->frustum_cam;
  half_v = cam_far * tanf(c->zoom * .5f);
  half_h = half_v * c->aspect;
  front = v3_mul(c->front, cam_far);
  eye = cam_get_eye(c);

  f->near = plane_new(v3_add(eye, v3_mul(c->front, cam_near)), c->front);
  f->far = plane_new(v3_add(eye, front), v3_neg(c->front));
  f->right = plane_new(eye, v3_cross(v3_sub(front, v3_mul(c->right, half_h)), c->up));
  f->right = plane_new(eye, v3_cross(c->up, v3_add(front, v3_mul(c->right, half_h))));
  f->top = plane_new(eye, v3_cross(c->right, v3_sub(front, v3_mul(c->up, half_v))));
  f->bottom = plane_new(eye, v3_cross(v3_add(front, v3_mul(c->up, half_v)), c->right));
}

void cam_rot(cam *c) {
  // ok how does this work?
  // we take the cosine of the yaw for x, that makes sense. then we multiply by
  //     the cosine of the pitch. this makes it project onto that vector.
  // for the y, we simply take the sine of the pitch.
  // we take the sine of the yaw for z, and project it onto the pitch vector.
  // ok, now it all makes sense!

  c->yaw = lerp(c->yaw, c->target_yaw, 0.5f);
  c->pitch = lerp(c->pitch, c->target_pitch, 0.5f);

  float yaw = c->yaw;
  float pitch = c->pitch;

  v3f front = v3_normed((v3f){
    cosf(rad(yaw)) * cosf(rad(pitch)),
    sinf(rad(pitch)),
    sinf(rad(yaw)) * cosf(rad(pitch))
  });

  c->front = front;

  v3f right = v3_cross(front, c->world_up);
  c->right = right;

  v3f up = v3_cross(right, front);
  c->up = up;

  m4f look = cam_get_look(c);
  m4f proj = cam_get_proj(c);
  c->vp = m4_mul(look, proj);

  float const overshoot_dist = 1.33f;
  float const overshoot_fov = 1.33f;

  look = m4_look(v3_sub(c->pos, v3_mul(c->front, c->dist * overshoot_dist)),
                 c->front,
                 c->up);
  proj = m4_persp(rad(c->zoom * overshoot_fov), c->aspect * overshoot_fov,
                  0.01f,
                  sqrtf(2.f) * 0.5f * chunk_size * world_draw_dist *
                  overshoot_dist);
  c->cvp = m4_mul(look, proj);

  cam_make_frustum(c);
}

int *quad_indices(int w, int h) {
  int *inds = arr_new(int, (w - 1) * (h - 1) * 6);

  for (int i = 0; i < h - 1; i++)
    for (int j = 0; j < w - 1; j++) {
      arr_add(&inds, &(int){(i + 1) * w + j + 1});
      arr_add(&inds, &(int){(i + 1) * w + j});
      arr_add(&inds, &(int){i * w + j});
      arr_add(&inds, &(int){i * w + j});
      arr_add(&inds, &(int){i * w + j + 1});
      arr_add(&inds, &(int){(i + 1) * w + j + 1});
    }

  return inds;
}

void imod_opti_vao(vao *v, buf *model) {
  gl_vertex_array_vertex_buffer(v->id, 1, model->id, 0, sizeof(m4f));
  for (int i = 0; i < 4; i++) {
    gl_enable_vertex_array_attrib(v->id, v->n_attrs + i);
    gl_vertex_array_attrib_format(v->id, v->n_attrs + i, 4, GL_FLOAT, GL_FALSE,
                                  i * sizeof(v4f));
    gl_vertex_array_attrib_binding(v->id, v->n_attrs + i, 1);
  }

  gl_vertex_array_binding_divisor(v->id, 1, 1);
}

static imod **all_imods = NULL;

imod *imod_new(mod m) {
  if (!all_imods) {
    all_imods = arr_new(imod *, 4);
  }

  imod out = {
    .meshes = m.meshes,
    .n_meshes = m.n_meshes,
    .n_texes = m.n_texes,
    .texes = m.texes,
    .model = arr_new(m4f, 4),
    .model_buf = buf_new(GL_ARRAY_BUFFER),
    .bounds = m.bounds
  };

  for (int i = 0; i < m.n_meshes; i++) {
    mesh *me = &m.meshes[i];
    imod_opti_vao(&me->vao, &out.model_buf);
  }

  imod *p = objdup(out);

  arr_add(&all_imods, &p);

  return p;
}

void imod_draw(draw_src s, cam *c) {
  if (!all_imods) return;

  for (imod **mp = all_imods, **end = arr_end(all_imods); mp != end; mp++) {
    imod *m = *mp;
    int count = arr_len(m->model);
    if (!count) continue;

    buf_data_n(&m->model_buf, GL_DYNAMIC_DRAW, sizeof(m4f), count, m->model);

    for (int i = 0; i < m->n_meshes; i++) {
      imod_get_sh(s, c, m->meshes[i].mat);
      (m->meshes[i].mat.cull ? gl_enable : gl_disable)(GL_CULL_FACE);

      vao_bind(&m->meshes[i].vao);
      gl_draw_elements_instanced(GL_TRIANGLES, m->meshes[i].n_inds,
                                 GL_UNSIGNED_INT, 0, count);

      the_app.n_tris += m->meshes[i].n_inds / 3 * count;
    }

    arr_clear(m->model);
  }

  gl_disable(GL_CULL_FACE);
}

void imod_add(imod *m, m4f t) {
  m4f t_tpose = m4_tpose(&t);
  arr_add(&m->model, &t_tpose);
}

shdr *imod_get_sh(draw_src s, cam *c, mtl m) {
  static shdr *cam = NULL;
  static shdr *shade = NULL;
  if (!cam) {
    cam = objdup(shdr_new(2, (shdr_spec[]){
      GL_VERTEX_SHADER, "res/imod.vsh",
      GL_FRAGMENT_SHADER, "res/mod_light.fsh"
    }));

    shade = objdup(shdr_new(2, (shdr_spec[]){
      GL_VERTEX_SHADER, "res/imod_depth.vsh",
      GL_FRAGMENT_SHADER, "res/mod_depth.fsh"
    }));
  }

  shdr *cur = s == ds_cam ? cam : shade;

  shdr_m4f(cur, "u_vp", c->vp);
  shdr_3f(cur, "u_eye", cam_get_eye(c));
  shdr_3f(cur, "u_light_model", m.light_model);
  shdr_3f(cur, "u_light", dreamy_haze[m.light]);
  shdr_3f(cur, "u_dark", dreamy_haze[m.dark]);
  shdr_1f(cur, "u_shine", m.shine);
  shdr_1f(cur, "u_wind", m.wind);
  shdr_1f(cur, "u_time", app_now() / 1000.f);
  shdr_bind(cur);

  return cur;
}

void crt_up(shdr *s, crt args) {
  shdr_bind(s);
  tex_bind(args.tex, args.unit);
  shdr_1i(s, "u_tex0", args.unit);
  shdr_1f(s, "u_aspect", args.aspect);
  shdr_1f(s, "u_lores", args.lores);
}

int cam_test_box(cam *c, box3 b, draw_src s) {
  return box3_viewable(b, s == ds_cam ? &c->frustum_cam : &c->frustum_shade);
}

void dof_up(shdr *s, dof args) {
  shdr_bind(s);
  tex_bind(args.tex, args.tex_unit);
  tex_bind(args.depth, args.depth_unit);
  shdr_1i(s, "u_tex", args.tex_unit);
  shdr_2f(s, "u_tex_size",
          (v2f){1.f / args.screen_size.x, 1.f / args.screen_size.y});
  shdr_1i(s, "u_depth", args.depth_unit);
  shdr_1i(s, "u_size", args.size);
  shdr_1f(s, "u_min_depth", args.min_depth);
  shdr_1f(s, "u_max_depth", args.max_depth);
  shdr_1f(s, "u_separation", args.separation);
  shdr_1f(s, "u_near", cam_near);
  shdr_1f(s, "u_far", cam_far);
}

v3f cam_get_eye(cam *c) {
  return v3_sub(c->pos, v3_mul(c->front, c->dist));
}

float plane_sdf(plane p, v3f pt) {
  return v3_dot(p.norm, pt) - p.dist;
}

plane plane_new(v3f point, v3f norm) {
  return (plane){
    .norm = norm,
    .dist = v3_dot(norm, point)
  };
}

tex_spec tex_spec_shadow(int width, int height, int filter) {
  return (tex_spec){
    .width = width, .height = height, .min_filter = filter, .mag_filter = filter,
    .internal_format = GL_DEPTH_COMPONENT32, .format = GL_DEPTH_COMPONENT, .pixels = NULL, .multisample = 0, .shadow = 1
  };
}
