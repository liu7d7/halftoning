#include "gl.h"
#include "err.h"
#include "game.h"

cam_t cam(float* pos, float* world_up, float yaw, float pitch) {
  const float default_speed = 2.5f, default_sens = 0.1f, default_zoom = 45.0f;

  cam_t c = {
    .front = {0, 0, -1},
    .yaw = yaw, .pitch = pitch, .target_yaw = yaw, .target_pitch = pitch,
    .speed = default_speed, .sens = default_sens, .zoom = default_zoom,
    .has_last = 0
  };

  vec3cpy(pos, c.pos);
  vec3cpy(world_up, c.world_up);
  vec3cpy(world_up, c.up);

  return c;
}

void cam_update(cam_t* c, game_t* g) {
  if (c->has_last && g->is_mouse_captured) {
    vec2 delta;
    glm_vec2_sub(g->mouse_pos, c->last_mouse_pos, delta);
    c->target_yaw += delta[0];
    c->target_pitch -= delta[1];

    if (c->target_pitch > 89.9f) c->target_pitch = 89.9f;
    if (c->target_pitch < -89.9f) c->target_pitch = -89.9f;
  }

  c->has_last = true;
  vec2cpy(g->mouse_pos, c->last_mouse_pos);

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
  vec3cpy(front, c->front);

  vec3 right;
  glm_cross(front, c->world_up, right);
  vec3cpy(right, c->right);

  vec3 up;
  glm_cross(right, front, up);
  vec3cpy(up, c->up);
}

void cam_look(cam_t* c, vec4* res) {
  glm_look(c->pos, c->front, c->up, res);
}

void cam_proj(cam_t* c, float const* win_size, vec4* res) {
  glm_perspective(to_rad(c->zoom), win_size[0] / win_size[1], 0.01f, 300.f, res);
}

void shader_verify(uint gl_id) {
  int is_ok;
  char info_log[1024];
  gl_get_shaderiv(gl_id, GL_COMPILE_STATUS, &is_ok);
  if (!is_ok) {
    gl_get_shader_info_log(gl_id, 1024, NULL, info_log);
    throw_nc(info_log);
  }
}

uint shader_compile(shader_conf_t s) {
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

  return gl_id;
}

void prog_verify(uint gl_id) {
  int is_ok;
  char info_log[1024];
  gl_get_programiv(gl_id, GL_LINK_STATUS, &is_ok);
  if (!is_ok) {
    gl_get_program_info_log(gl_id, 1024, NULL, info_log);
    throw_nc(info_log);
  }
}

shader_t shader(uint n, shader_conf_t* shaders) {
  uint gl_ids[n], gl_id = gl_create_program();

  for (int i = 0; i < n; i++) {
    gl_ids[i] = shader_compile(shaders[i]);
    gl_attach_shader(gl_id, gl_ids[i]);
  }

  gl_link_program(gl_id);
  prog_verify(gl_id);

  return (shader_t){.id = gl_id};
}

vao_t vao(buf_t* vbo, buf_t* ibo, uint n, attrib_t* attrs) {
  vao_t v = {.id = 0, .vbo = vbo, .ibo = ibo};
  gl_create_vertex_arrays(1, &v.id);

  int stride = 0;
  for (int i = 0; i < n; i++) {
    attrib_t a = attrs[i];
    stride += a.size * (int)(a.type == GL_INT ? sizeof(int) : sizeof(float));
  }

  gl_vertex_array_vertex_buffer(v.id, 0, v.vbo->id, 0, stride);

  int offset = 0;
  for (int i = 0; i < n; i++) {
    attrib_t a = attrs[i];
    gl_enable_vertex_array_attrib(v.id, i);
    if (a.type == GL_FLOAT) {
      gl_vertex_array_attrib_format(v.id, i, a.size, GL_FLOAT, GL_FALSE, offset);
    } else {
      gl_vertex_array_attrib_i_format(v.id, i, a.size, GL_INT, offset);
    }

    gl_vertex_array_attrib_binding(v.id, i, 0);
    offset += a.size;
  }

  if (v.ibo) gl_vertex_array_element_buffer(v.id, v.ibo->id);

  return v;
}

buf_t buf(uint type) {
  buf_t b = {.id = 0, .type = type};
  gl_create_buffers(1, &b.id);

  return b;
}

buf_t* buf_heap_new(uint type) {
  buf_t b = buf(type);
  return memcpy(malloc(sizeof(b)), &b, sizeof(b));
}

void buf_data_n(buf_t* b, uint usage, ssize_t elem_size, ssize_t n, void* data) {
  buf_data(b, usage, n * elem_size, data);
}

void buf_data(buf_t* b, uint usage, ssize_t size_in_bytes, void* data) {
  gl_named_buffer_storage(b->id, size_in_bytes, data, usage);
}

void shader_bind(shader_t* s) {
  gl_use_program(s->id);
}

void vao_bind(vao_t* v) {
  buf_bind(v->vbo);
  if (v->ibo) buf_bind(v->ibo);

  gl_bind_vertex_array(v->id);
}

void buf_bind(buf_t* b) {
  gl_bind_buffer(b->type, b->id);
}

void shader_mat4(shader_t* s, char const* n, mat4 m) {
  shader_bind(s);
  gl_uniform_matrix_4fv(gl_get_uniform_location(s->id, n), 1, GL_TRUE, &m[0][0]);
}

void shader_int(shader_t* s, char const* n, int m) {
  shader_bind(s);
  gl_uniform_1i(gl_get_uniform_location(s->id, n), m);
}

void shader_float(shader_t* s, char const* n, float m) {
  shader_bind(s);
  gl_uniform_1f(gl_get_uniform_location(s->id, n), m);
}

void shader_vec2(shader_t* s, char const* n, float* m) {
  shader_bind(s);
  gl_uniform_2f(gl_get_uniform_location(s->id, n), m[0], m[1]);
}

void shader_vec3(shader_t* s, char const* n, float* m) {
  shader_bind(s);
  gl_uniform_3f(gl_get_uniform_location(s->id, n), m[0], m[1], m[2]);
}

void shader_vec4(shader_t* s, char const* n, float* m) {
  shader_bind(s);
  gl_uniform_4f(gl_get_uniform_location(s->id, n), m[0], m[1], m[2], m[3]);
}

