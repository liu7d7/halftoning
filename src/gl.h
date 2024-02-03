#pragma once

#include "lib/glad/glad.h"
#include <intrin.h>
#include "typedefs.h"
#include <cglm/cglm.h>

// forward declaration
struct game_t;

typedef struct cam_t {
  vec3 pos, front, up, right, world_up;
  float target_yaw, yaw, target_pitch, pitch, speed, vel, sens, zoom;

  vec2 last_mouse_pos;
  bool has_last;
} cam_t;

cam_t cam(vec3 pos, vec3 world_up, float yaw, float pitch);
void cam_update(cam_t* c, struct game_t* g);
void cam_look(cam_t* c, mat4 res);
void cam_proj(cam_t* c, float const* win_size, mat4 res);

typedef struct shader_t {
  uint id;
} shader_t;

typedef struct shader_conf_t {
  char const* path;
  uint type;
} shader_conf_t;

shader_t shader(uint n, shader_conf_t* shaders);
void shader_bind(shader_t* s);
void shader_mat4(shader_t* s, char const* n, mat4 m);
void shader_int(shader_t* s, char const* n, int m);
void shader_float(shader_t* s, char const* n, float m);
void shader_vec2(shader_t* s, char const* n, vec2 m);
void shader_vec3(shader_t* s, char const* n, vec3 m);
void shader_vec4(shader_t* s, char const* n, vec4 m);

typedef struct buf_t {
  uint id;
  uint type;
} buf_t;

buf_t buf(uint type);
buf_t* buf_heap_new(uint type);
void buf_data_n(buf_t* b, uint usage, ssize_t elem_size, ssize_t n, void* data);
void buf_data(buf_t* b, uint usage, ssize_t size_in_bytes, void* data);
void buf_bind(buf_t* b);

typedef struct vao_t {
  uint id;

  // owning!
  buf_t* vbo;

  // owning!
  // can be null!
  buf_t* ibo;
} vao_t;

void vao_bind(vao_t* v);

typedef struct attrib_t {
  int size;
  uint type;
} attrib_t;

static attrib_t attr_float_1 = {1, GL_FLOAT};
static attrib_t attr_float_2 = {2, GL_FLOAT};
static attrib_t attr_float_3 = {3, GL_FLOAT};
static attrib_t attr_float_4 = {4, GL_FLOAT};

static attrib_t attr_int_1 = {1, GL_INT};
static attrib_t attr_int_2 = {2, GL_INT};
static attrib_t attr_int_3 = {3, GL_INT};
static attrib_t attr_int_4 = {4, GL_INT};

vao_t
vao(buf_t* vbo, buf_t* ibo, uint n, attrib_t* attrs);