#pragma once

#include <stdint-gcc.h>
#include <intrin.h>
#include <cglm/cglm.h>
#include <stdbool.h>

typedef uint32_t uint;
typedef uint8_t byte;

[[gnu::always_inline]]

inline static void vec3cpy(vec3 dst, vec3 src) {
  glm_vec3_copy(src, dst);
}

[[gnu::always_inline]]

inline static void vec3inc(vec3 a, vec3 b) {
  glm_vec3_add(a, b, a);
}

[[gnu::always_inline]]

inline static void vec2cpy(vec2 dst, vec2 src) {
  glm_vec2_copy(src, dst);
}

[[gnu::always_inline]]

inline static float to_rad(float theta) {
  return glm_rad(theta);
}

#define VEC2_COPY_INIT(vec) {(vec)[0], (vec)[1]}
#define VEC3_COPY_INIT(vec) {(vec)[0], (vec)[1], (vec)[2]}

typedef int ivec2[2];

static size_t ivec2_hash(void* key) {
  int* vec = key;

  return ((size_t)vec[0] << 32) | ((size_t)vec[1]);
}

static bool ivec2_eq(void* _lhs, void* _rhs) {
  int* lhs = _lhs;
  int* rhs = _rhs;

  return lhs[0] == rhs[0] && lhs[1] == rhs[1];
}