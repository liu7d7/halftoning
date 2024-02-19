#include "gl.h"
#include "err.h"
#include "game.h"
#include "dyn_arr.h"

struct cam
cam(float* pos, float* world_up, float yaw, float pitch, float aspect) {
  const float default_speed = 2.5f, default_sens = 0.1f, default_zoom = 45.0f;

  struct cam c = {
    .front = {0, 0, -1},
    .yaw = yaw, .pitch = pitch, .target_yaw = yaw, .target_pitch = pitch,
    .speed = default_speed, .sens = default_sens, .zoom = default_zoom,
    .has_last = 0,
    .aspect = aspect
  };

  vec3cpy(c.pos, pos);
  vec3cpy(c.world_up, world_up);
  vec3cpy(c.up, world_up);

  return c;
}

void cam_update(struct cam* c, struct game* g) {
  if (c->has_last && g->is_mouse_captured) {
    vec2 delta;
    glm_vec2_sub(g->mouse_pos, c->last_mouse_pos, delta);
    c->target_yaw += delta[0];
    c->target_pitch -= delta[1];

    if (c->target_pitch > 89.9f) c->target_pitch = 89.9f;
    if (c->target_pitch < -89.9f) c->target_pitch = -89.9f;
  }

  c->has_last = true;
  vec2cpy(c->last_mouse_pos, g->mouse_pos);

  c->yaw = glm_lerp(c->yaw, c->target_yaw, 0.5f);
  c->pitch = glm_lerp(c->pitch, c->target_pitch, 0.5f);

  // ok how does this work?
  // we take the cosine of the yaw for x, that makes sense. then we multiply by
  //     the cosine of the pitch. this makes it project onto that vector.
  // for the y, we simply take the sine of the pitch.
  // we take the sine of the yaw for z, and project it onto the pitch vector.
  // ok, now it all makes sense!

  vec3 front = {
    cosf(to_rad(c->yaw)) * cosf(to_rad(c->pitch)),
    sinf(to_rad(c->pitch)),
    sinf(to_rad(c->yaw)) * cosf(to_rad(c->pitch))
  };

  glm_normalize(front);
  vec3cpy(c->front, front);

  vec3 right;
  glm_cross(front, c->world_up, right);
  vec3cpy(c->right, right);

  vec3 up;
  glm_cross(right, front, up);
  vec3cpy(c->up, up);
}

void cam_look(struct cam* c, vec4* res) {
  glm_look(c->pos, c->front, c->up, res);
}

void cam_proj(struct cam* c, vec4* res) {
  glm_perspective(to_rad(c->zoom), c->aspect, 0.01f, 3000.f,
                  res);
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

uint shader_compile(struct shader_spec s) {
  FILE* f = fopen(s.path, "r");
  if (!f) {
    throw_c("Failed to open file for shader_component!");
  }

  char* src = calloc(1 << 20, 1);
  char block[64];
  while (fgets(block, sizeof(block), f)) {
    strcat(src, block);
  }

  uint gl_id = gl_create_shader(s.type);
  gl_shader_source(gl_id, 1, (char const* []){src},
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

struct shader shader(uint n, struct shader_spec* shaders) {
  uint gl_ids[n], gl_id = gl_create_program();

  for (int i = 0; i < n; i++) {
    gl_ids[i] = shader_compile(shaders[i]);
    gl_attach_shader(gl_id, gl_ids[i]);
  }

  gl_link_program(gl_id);
  prog_verify(gl_id);

  return (struct shader){.id = gl_id};
}

struct vao vao(struct buf* vbo, struct buf* ibo, uint n, struct attrib* attrs) {
  struct vao v = {.id = 0};
  gl_create_vertex_arrays(1, &v.id);

  int stride = 0;
  for (int i = 0; i < n; i++) {
    struct attrib a = attrs[i];
    stride += a.size * (int)(a.type == GL_INT ? sizeof(int) : sizeof(float));
  }

  gl_vertex_array_vertex_buffer(v.id, 0, vbo->id, 0, stride);

  int offset = 0;
  for (int i = 0; i < n; i++) {
    struct attrib a = attrs[i];
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

struct buf buf(uint type) {
  struct buf b = {.id = 0, .type = type};
  gl_create_buffers(1, &b.id);

  return b;
}

struct buf* buf_heap_new(uint type) {
  struct buf b = buf(type);
  return memcpy(malloc(sizeof(b)), &b, sizeof(b));
}

void buf_data_n(struct buf* b, uint usage, ssize_t elem_size, ssize_t n,
                void* data) {
  buf_data(b, usage, n * elem_size, data);
}

void buf_data(struct buf* b, uint usage, ssize_t size_in_bytes, void* data) {
  gl_named_buffer_storage(b->id, size_in_bytes, data, usage);
}

void shader_bind(struct shader* s) {
  gl_use_program(s->id);
}

void vao_bind(struct vao* v) {
  gl_bind_vertex_array(v->id);
}

void buf_bind(struct buf* b) {
  gl_bind_buffer(b->type, b->id);
}

void shader_mat4(struct shader* s, char const* n, mat4 m) {
  shader_bind(s);
  gl_uniform_matrix_4fv(gl_get_uniform_location(s->id, n), 1, GL_TRUE,
                        &m[0][0]);
}

void shader_int(struct shader* s, char const* n, int m) {
  shader_bind(s);
  gl_uniform_1i(gl_get_uniform_location(s->id, n), m);
}

void shader_float(struct shader* s, char const* n, float m) {
  shader_bind(s);
  gl_uniform_1f(gl_get_uniform_location(s->id, n), m);
}

void shader_vec2(struct shader* s, char const* n, float* m) {
  shader_bind(s);
  gl_uniform_2f(gl_get_uniform_location(s->id, n), m[0], m[1]);
}

void shader_vec3(struct shader* s, char const* n, float* m) {
  shader_bind(s);
  gl_uniform_3f(gl_get_uniform_location(s->id, n), m[0], m[1], m[2]);
}

void shader_vec4(struct shader* s, char const* n, float* m) {
  shader_bind(s);
  gl_uniform_4f(gl_get_uniform_location(s->id, n), m[0], m[1], m[2], m[3]);
}

int attrib_get_size_in_bytes(struct attrib* attr) {
  return attr->size * (int)(attr->type == GL_INT ? sizeof(int) : sizeof(float));
}

struct tex_spec tex_spec_invalid() {
  return (struct tex_spec){0};
}

struct tex_spec tex_spec_rgba8(int width, int height, int filter) {
  return (struct tex_spec){
    .width = width, .height = height, .min_filter = filter, .mag_filter = filter,
    .internal_format = GL_RGBA8, .format = GL_RGBA, .pixels = NULL
  };
}

struct tex_spec tex_spec_rgba16(int width, int height, int filter) {
  return (struct tex_spec){
    .width = width, .height = height, .min_filter = filter, .mag_filter = filter,
    .internal_format = GL_RGBA16F, .format = GL_RGBA, .pixels = NULL
  };
}

struct tex_spec tex_spec_r16(int width, int height, int filter) {
  return (struct tex_spec){
    .width = width, .height = height, .min_filter = filter, .mag_filter = filter,
    .internal_format = GL_R16F, .format = GL_RED, .pixels = NULL
  };
}

struct tex_spec tex_spec_depth24(int width, int height, int filter) {
  return (struct tex_spec){
    .width = width, .height = height, .min_filter = filter, .mag_filter = filter,
    .internal_format = GL_DEPTH_COMPONENT32, .format = GL_DEPTH_COMPONENT, .pixels = NULL
  };
}

struct tex tex(struct tex_spec spec) {
  struct tex t = {.id = 0, .spec = spec};
  gl_create_textures(GL_TEXTURE_2D, 1, &t.id);
  gl_texture_parameteri(t.id, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
  gl_texture_parameteri(t.id, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  gl_texture_parameteri(t.id, GL_TEXTURE_MIN_FILTER, spec.min_filter);
  gl_texture_parameteri(t.id, GL_TEXTURE_MAG_FILTER, spec.mag_filter);

  gl_texture_storage_2d(t.id, 1, spec.internal_format, spec.width, spec.height);

  if (spec.pixels) {
    gl_texture_sub_image_2d(t.id, 0, 0, 0, spec.width, spec.height, spec.format,
                            GL_UNSIGNED_BYTE, spec.pixels);
  }

  return t;
}

void tex_resize(struct tex* t, int width, int height) {
  if (t->spec.pixels) {
    throw_c("Can't resize a texture that specifies its pixels!");
  }

  struct tex_spec resized_spec = t->spec;
  resized_spec.width = width, resized_spec.height = height;
  struct tex resized = tex(resized_spec);

  gl_delete_textures(1, &t->id);
  *t = resized;
}

void tex_bind(struct tex* t, uint unit) {
  if (unit >= 16) throw_c("Unit too high!");
  gl_active_texture(unit + GL_TEXTURE0);
  gl_bind_texture(GL_TEXTURE_2D, t->id);
}

void tex_del(struct tex* t) {
  gl_delete_textures(1, &t->id);
  if (t->spec.pixels) {
    free(t->spec.pixels);
    t->spec.pixels = NULL;
  }
}

struct fbo fbo(uint n, struct fbo_spec* spec) {
  struct fbo f = {.id = 0, .bufs = malloc(
    n * sizeof(struct fbo_buf)), .n_bufs = n};
  gl_create_framebuffers(1, &f.id);
  for (int i = 0; i < n; i++) {
    struct fbo_spec s = spec[i];
    struct tex t = tex(s.spec);
    f.bufs[i] = (struct fbo_buf){.id = s.id, .tex = t};
    gl_named_framebuffer_texture(f.id, s.id, t.id, 0);
  }

  return f;
}

void fbo_bind(struct fbo* f) {
  gl_bind_framebuffer(GL_FRAMEBUFFER, f->id);
}

void fbo_draw_bufs(struct fbo* f, int n, uint* bufs) {
  gl_named_framebuffer_draw_buffers(f->id, n, bufs);
}

void fbo_read_buf(struct fbo* f, uint buf) {
  gl_named_framebuffer_read_buffer(f->id, buf);
}

bool is_gl_buf_color_attachment(uint it) {
  return it >= GL_COLOR_ATTACHMENT0 && it <= GL_COLOR_ATTACHMENT31;
}

struct tex* fbo_tex_at(struct fbo* f, uint buf) {
  for (int i = 0; i < f->n_bufs; i++) {
    if (f->bufs[i].id == buf) {
      return &f->bufs[i].tex;
    }
  }

  throw_c("Failed to find attachment of framebuffer!");
}

void
fbo_blit(struct fbo* src, struct fbo* dst, uint src_a, uint dst_a,
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

  struct tex* src_tex = fbo_tex_at(src, src_a);
  struct tex* dst_tex = fbo_tex_at(dst, dst_a);

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

void fbo_resize(struct fbo* f, int width, int height, uint n, uint* bufs) {
  for (int i = 0; i < n; i++) {
    struct tex* t = fbo_tex_at(f, bufs[i]);
    tex_resize(t, width, height);
    gl_named_framebuffer_texture(f->id, bufs[i], t->id, 0);
  }
}

void to_cmyk_up(struct shader* s, struct to_cmyk args) {
  shader_bind(s);
  tex_bind(args.tex, args.unit);
  shader_int(s, "u_tex", args.unit);
}

void halftone_up(struct shader* s, struct halftone args) {
  shader_bind(s);
  tex_bind(args.cmyk, args.unit);
  shader_int(s, "u_cmyk", args.unit);
  shader_vec2(s, "u_scr_size", args.scr_size);
  shader_float(s, "u_dots_per_line", (float)args.dots_per_line);
}

void blit_up(struct shader* s, struct blit args) {
  shader_bind(s);
  tex_bind(args.tex, args.unit);
  shader_int(s, "u_tex", args.unit);
}

void blur_up(struct shader* s, struct blur args) {
  shader_bind(s);
  tex_bind(args.tex, args.unit);
  shader_int(s, "u_tex", args.unit);
  shader_vec2(s, "u_scr_size", args.scr_size);
}

struct mesh
mod_load_mesh(struct mod* m, struct aiMesh* mesh, const struct aiScene* scene) {
  struct mod_vtx* vtxs = malloc(sizeof(struct mod_vtx) * mesh->mNumVertices);

  for (int i = 0; i < mesh->mNumVertices; i++) {
    vec3 pos, norm;
    vec2 uvs = GLM_VEC2_ZERO_INIT;
    vec3cpy(pos, (float*)&mesh->mVertices[i]);
    vec3cpy(norm, (float*)&mesh->mNormals[i]);
    if (mesh->mTextureCoords[0]) {
      vec2cpy(uvs, (float*)&mesh->mTextureCoords[0][i]);
    }

    vtxs[i] = mod_vtx(pos, norm, uvs);
  }

  struct buf vbo = buf(GL_ARRAY_BUFFER), ibo = buf(GL_ELEMENT_ARRAY_BUFFER);

  buf_data_n(&vbo,
             GL_DYNAMIC_STORAGE_BIT,
             sizeof(struct mod_vtx),
             mesh->mNumVertices,
             vtxs);

  uint* inds = dyn_arr(uint, 4);
  for (int i = 0; i < mesh->mNumFaces; i++) {
    for (int j = 0; j < mesh->mFaces[i].mNumIndices; j++) {
      dyn_arr_add(inds, &mesh->mFaces[i].mIndices[j]);
    }
  }

  buf_data_n(&ibo,
             GL_DYNAMIC_STORAGE_BIT,
             sizeof(uint),
             dyn_arr_count(inds),
             inds);

  struct mesh me = {
    .vtxs = vtxs,
    .n_vtxs = (int)mesh->mNumVertices,
    .n_inds = dyn_arr_count(inds),
    .vao = vao(&vbo, &ibo, 3,
               (struct attrib[]){attr_3f, attr_3f, attr_2f})
  };

  dyn_arr_del(inds);

  return me;
}

void mod_load(struct mod* m, struct aiNode* node, const struct aiScene* scene) {
  for (int i = 0; i < node->mNumMeshes; i++) {
    struct aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    m->meshes[m->n_meshes++] = mod_load_mesh(m, mesh, scene);
  }

  for (int i = 0; i < node->mNumChildren; i++) {
    mod_load(m, node->mChildren[i], scene);
  }
}

struct mod mod(const char* path) {
  struct aiScene const* scene =
    aiImportFile(path,
                 aiProcess_CalcTangentSpace
                 | aiProcess_Triangulate
                 | aiProcess_JoinIdenticalVertices);
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    throw_c(aiGetErrorString());
  }

  struct mod m = {
    .meshes = malloc(sizeof(struct mesh) * scene->mNumMeshes)
  };

  mod_load(&m, scene->mRootNode, scene);

  return m;
}

void mod_draw(struct mod* m, struct cam* c) {
  for (int i = 0; i < m->n_meshes; i++) {
    (void)mod_get_shader(c);

    vao_bind(&m->meshes[i].vao);
    gl_draw_elements(GL_TRIANGLES, m->meshes[i].n_inds, GL_UNSIGNED_INT, 0);
  }
}

struct mod_vtx mod_vtx(float* pos, float* norm, float* uvs) {
  struct mod_vtx vtx;

  vec3cpy(vtx.pos, pos);
  vec3cpy(vtx.norm, norm);
  vec2cpy(vtx.uvs, uvs);

  return vtx;
}

struct shader* mod_get_shader(struct cam* c) {
  static struct shader* sh = NULL;
  if (!sh) {
    struct shader s = shader(2,
                             (struct shader_spec[]){
                               {GL_VERTEX_SHADER,   "res/mod.vsh"},
                               {GL_FRAGMENT_SHADER, "res/mod.fsh"},
                             });
    sh = memcpy(malloc(sizeof(s)), &s, sizeof(s));
  }

  mat4 mat;
  cam_proj(c, mat);
  shader_mat4(sh, "u_proj", mat);
  cam_look(c, mat);
  shader_mat4(sh, "u_look", mat);
  shader_vec3(sh, "u_eye", c->pos);
  shader_bind(sh);

  return sh;
}
