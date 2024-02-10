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

inline static void vec2cpy(vec2 dst, vec2 src) {
  glm_vec2_copy(src, dst);
}

[[gnu::always_inline]]
inline static float to_rad(float theta) {
  return glm_rad(theta);
}

#define VEC2_COPY_INIT(vec) {(vec)[0], (vec)[1]}