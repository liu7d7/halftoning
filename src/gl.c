#include "gl.h"
#include "err.h"
#include "app.h"
#include "arr.h"

cam
cam_new(v3f pos, v3f world_up, float yaw, float pitch, float aspect) {
  const float default_speed = 2.5f, default_sens = 0.1f, default_zoom = 45.0f;

  cam c = {
    .front = {0, 0, -1},
    .yaw = yaw, .pitch = pitch, .prev_yaw = yaw, .prev_pitch = pitch,
    .speed = default_speed, .sens = default_sens, .zoom = default_zoom,
    .has_last = 0,
    .aspect = aspect,
    .pos = pos,
    .prev_pos = pos,
    .world_up = world_up,
    .up = world_up
  };

  return c;
}

void cam_tick(cam *c, app *g) {
  c->prev_pitch = c->pitch;
  c->prev_yaw = c->yaw;

  if (c->has_last && g->is_mouse_captured) {
    v2f delta = v2_sub(g->mouse, c->last_mouse_pos);
    c->yaw += delta.x;
    c->pitch -= delta.y;

    if (c->pitch > 89.9f) c->pitch = 89.9f;
    if (c->pitch < -89.9f) c->pitch = -89.9f;
  }

  c->has_last = true;
  c->last_mouse_pos = g->mouse;
}

m4f cam_get_look(cam *c, float d) {
  return m4_look(cam_get_pos(c, d), c->front, c->up);
}

m4f cam_get_proj(cam *c) {
  return m4_persp(rad(c->zoom), c->aspect, 0.01f, 144.f);
}

void shader_verify(uint gl_id) {
  int is_ok;
  char info_log[1024];
  gl_get_shaderiv(gl_id, GL_COMPILE_STATUS, &is_ok);
  if (!is_ok) {
    gl_get_shader_info_log(gl_id, 1024, NULL, info_log);
    throw_c(info_log);
  }
}

uint shader_compile(shader_spec s) {
  FILE *f = fopen(s.path, "r");
  if (!f) {
    throw_c("Failed to open file for shader_component!");
  }

  char *src = calloc(1 << 20, 1);
  char block[64];
  while (fgets(block, sizeof(block), f)) {
    strcat(src, block);
  }

  uint gl_id = gl_create_shader(s.type);
  gl_shader_source(gl_id, 1, (char const *[]){src},
                   (int[]){(int)strlen(src)});
  gl_compile_shader(gl_id);
  shader_verify(gl_id);

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

shader shader_new(uint n, shader_spec *shaders) {
  uint gl_ids[n], gl_id = gl_create_program();

  for (int i = 0; i < n; i++) {
    gl_ids[i] = shader_compile(shaders[i]);
    gl_attach_shader(gl_id, gl_ids[i]);
  }

  gl_link_program(gl_id);
  prog_verify(gl_id);

  return (shader){.id = gl_id};
}

struct vao vao_new(buf *vbo, buf *ibo, uint n, attrib *attrs) {
  struct vao v = {.id = 0};
  gl_create_vertex_arrays(1, &v.id);

  int stride = 0;
  for (int i = 0; i < n; i++) {
    attrib a = attrs[i];
    stride += a.size * (int)(a.type == GL_INT ? sizeof(int) : sizeof(float));
  }

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

void buf_data_n(buf *b, uint usage, ssize_t elem_size, ssize_t n,
                void *data) {
  buf_data(b, usage, n * elem_size, data);
}

void buf_data(buf *b, uint usage, ssize_t size_in_bytes, void *data) {
  gl_named_buffer_data(b->id, size_in_bytes, data, usage);
}

void shader_bind(shader *s) {
  gl_use_program(s->id);
}

void vao_bind(struct vao *v) {
  gl_bind_vertex_array(v->id);
}

void buf_bind(buf *b) {
  gl_bind_buffer(b->type, b->id);
}

void shader_mat4(shader *s, char const *n, m4f m) {
  shader_bind(s);
  gl_uniform_matrix_4fv(gl_get_uniform_location(s->id, n), 1, GL_TRUE,
                        &m.v[0][0]);
}

void shader_int(shader *s, char const *n, int m) {
  shader_bind(s);
  gl_uniform_1i(gl_get_uniform_location(s->id, n), m);
}

void shader_float(shader *s, char const *n, float m) {
  shader_bind(s);
  gl_uniform_1f(gl_get_uniform_location(s->id, n), m);
}

void shader_vec2(shader *s, char const *n, v2f m) {
  shader_bind(s);
  gl_uniform_2f(gl_get_uniform_location(s->id, n), m.x, m.y);
}

void shader_vec3(shader *s, char const *n, v3f m) {
  shader_bind(s);
  gl_uniform_3f(gl_get_uniform_location(s->id, n), m.x, m.y, m.z);
}

void shader_vec4(shader *s, char const *n, v4f m) {
  shader_bind(s);
  gl_uniform_4f(gl_get_uniform_location(s->id, n), m.x, m.y, m.z, m.w);
}

int attrib_get_size_in_bytes(attrib *attr) {
  return attr->size * (int)(attr->type == GL_INT ? sizeof(int) : sizeof(float));
}

tex_spec tex_spec_invalid() {
  return (tex_spec){0};
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

tex_spec tex_spec_depth24(int width, int height, int filter) {
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
    gl_texture_parameteri(t.id, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    gl_texture_parameteri(t.id, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
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

void to_cmyk_up(shader *s, to_cmyk args) {
  shader_bind(s);
  tex_bind(args.tex, args.unit);
  shader_int(s, "u_tex", args.unit);
}

void halftone_up(shader *s, halftone args) {
  shader_bind(s);
  tex_bind(args.cmyk, args.unit);
  shader_int(s, "u_cmyk", args.unit);
  shader_vec2(s, "u_scr_size", args.scr_size);
  shader_float(s, "u_dots_per_line", (float)args.dots_per_line);
}

void blit_up(shader *s, blit args) {
  shader_bind(s);
  tex_bind(args.tex, args.unit);
  shader_int(s, "u_tex", args.unit);
}

void blur_up(shader *s, blur args) {
  shader_bind(s);
  tex_bind(args.tex, args.unit);
  shader_int(s, "u_tex", args.unit);
  shader_vec2(s, "u_scr_size", args.scr_size);
}

mesh
mod_load_mesh(mod *m, struct aiMesh *mesh, const struct aiScene *scene) {
  mod_vtx *vtxs = malloc(sizeof(mod_vtx) * mesh->mNumVertices);

  for (int i = 0; i < mesh->mNumVertices; i++) {
    v3f pos = *(v3f *)&mesh->mVertices[i], norm = *(v3f *)&mesh->mNormals[i];
    v2f uvs = {0};
    if (mesh->mTextureCoords[0]) {
      uvs = *(v2f *)&mesh->mTextureCoords[0][i];
    }

    vtxs[i] = (mod_vtx){pos, norm, uvs};
  }

  buf vbo = buf_new(GL_ARRAY_BUFFER), ibo = buf_new(GL_ELEMENT_ARRAY_BUFFER);

  buf_data_n(&vbo,
             GL_DYNAMIC_DRAW,
             sizeof(mod_vtx),
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
    .vao = vao_new(&vbo, &ibo, 3,
                   (attrib[]){attr_3f, attr_3f, attr_2f})
  };

  arr_del(inds);

  return me;
}

void mod_load(mod *m, struct aiNode *node, const struct aiScene *scene) {
  for (int i = 0; i < node->mNumMeshes; i++) {
    struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    m->meshes[m->n_meshes++] = mod_load_mesh(m, mesh, scene);
  }

  for (int i = 0; i < node->mNumChildren; i++) {
    mod_load(m, node->mChildren[i], scene);
  }
}

mod mod_new(const char *path) {
  struct aiScene const *scene =
    aiImportFile(path,
                 aiProcess_CalcTangentSpace
                 | aiProcess_Triangulate
                 | aiProcess_JoinIdenticalVertices);
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    throw_c(aiGetErrorString());
  }

  mod m = {
    .meshes = malloc(sizeof(mesh) * scene->mNumMeshes)
  };

  mod_load(&m, scene->mRootNode, scene);

  aiReleaseImport(scene);

  return m;
}

void mod_draw(mod *m, cam *c, m4f t, float d) {
  for (int i = 0; i < m->n_meshes; i++) {
    (void)mod_get_shader(c, t, d);

    vao_bind(&m->meshes[i].vao);
    gl_draw_elements(GL_TRIANGLES, m->meshes[i].n_inds, GL_UNSIGNED_INT, 0);
  }
}

shader *mod_get_shader(cam *c, m4f t, float d) {
  static shader *sh = NULL;
  if (!sh) {
    sh = objdup(shader_new(2,
                           (shader_spec[]){
                             {GL_VERTEX_SHADER,   "res/mod.vsh"},
                             {GL_FRAGMENT_SHADER, "res/mod_light.fsh"},
                           }));
  }

  m4f t_no_scale = t;

  for (int i = 0; i < 3; i++) {
    v3f a = {t.v[i][0], t.v[i][1], t.v[i][2]};
    v3_norm(&a);
    t_no_scale.v[i][0] = a.v[0], t_no_scale.v[i][1] = a.v[1], t_no_scale.v[i][2] = a.v[2];
  }

  m4f proj = cam_get_proj(c), look = cam_get_look(c, d);
  shader_mat4(sh, "u_proj", proj);
  shader_mat4(sh, "u_look", look);
  shader_vec3(sh, "u_eye", c->pos);
  shader_mat4(sh, "u_model", t);
  shader_mat4(sh, "u_model_no_scale", t_no_scale);
  shader_bind(sh);

  return sh;
}

void cam_rot(cam *c, float d) {
  // ok how does this work?
  // we take the cosine of the yaw for x, that makes sense. then we multiply by
  //     the cosine of the pitch. this makes it project onto that vector.
  // for the y, we simply take the sine of the pitch.
  // we take the sine of the yaw for z, and project it onto the pitch vector.
  // ok, now it all makes sense!

  float yaw = cam_get_yaw(c, d);
  float pitch = cam_get_pitch(c, d);

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
}

v3f cam_get_pos(cam *c, float d) {
  return v3_lerp(c->prev_pos, c->pos, d);
}

float cam_get_yaw(cam *c, float d) {
  return lerp(c->prev_yaw, c->yaw, d);
}

float cam_get_pitch(cam *c, float d) {
  return lerp(c->prev_pitch, c->pitch, d);
}

int *quad_indices(int w, int h) {
  int *inds = arr_new(int, w * h * 6);

  for (int i = 0; i < h; i++)
    for (int j = 0; j < w; j++)
    {
      arr_add(&inds, &(int){i * (w + 1) + j});
      arr_add(&inds, &(int){(i + 1) * (w + 1) + j + 1});
      arr_add(&inds, &(int){(i + 1) * (w + 1) + j});
      arr_add(&inds, &(int){i * (w + 1) + j});
      arr_add(&inds, &(int){i * (w + 1) + j + 1});
      arr_add(&inds, &(int){(i + 1) * (w + 1) + j + 1});
    }

  return inds;
}
