#pragma once

#include <stdint-gcc.h>
#include <intrin.h>
#include <cglm/cglm.h>
#include <stdbool.h>

typedef uint32_t uint;

[[gnu::always_inline]]

inline static void vec3cpy(vec3 src, vec3 dst) {
  glm_vec3_copy(src, dst);
}

[[gnu::always_inline]]

inline static void vec2cpy(vec2 src, vec2 dst) {
  glm_vec2_copy(src, dst);
}

[[gnu::always_inline]]
inline static float to_rad(float theta) {
  return glm_rad(theta);
}