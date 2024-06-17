#pragma once

#ifdef __GNUC__

#include <stdint-gcc.h>

#else
#include <stdint.h>
#endif

#include <intrin.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "err.h"
#include "hash.h"

typedef uint32_t u32;
typedef uint8_t u8;
#ifndef __GNUC__
typedef int64_t ssize_t;
#endif

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define _new_(...) \
  ({ __typeof__ (__VA_ARGS__) _a = (__VA_ARGS__); \
     (__typeof__ (__VA_ARGS__) *)memcpy(malloc(sizeof(_a)), &_a, sizeof(_a));\
  })

#define _swap_(a, b) \
  ({__typeof__ (*a) temp = *a; *a = *b; *b = temp; })
#define M_PIF 3.1415926f

static u8 *read_bin_file(char const *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    throwf("read_bin_file: failed to open file at path!");
  }

  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  rewind(f);

  u8 *buf = malloc(size);
  if (!fread(buf, size, 1, f)) {
    fclose(f);
    free(buf);
    throwf("read_bin_file: failed to read file!");
  }

  fclose(f);
  return buf;
}

static char *read_txt_file_len(char const *path, size_t *len) {
  FILE *file = fopen(path, "r");
  char *result = 0;
  if (!file) {
    throwf("read_txt_file: failed to read text file at %s!", path);
  }

  size_t file_size = 0;
  fseek(file, 0L, SEEK_END);
  file_size = (size_t)ftell(file);
  fseek(file, 0L, SEEK_SET);
  result = (char *)calloc(file_size + 1, sizeof(char));
  if (!fread(result, sizeof(char), file_size, file)) {
    throwf("read_txt_file: failed to read from text file!");
  }

  result[file_size] = '\0';
  fclose(file);

  if (len) *len = file_size;
  return result;
}

static char *read_txt_file(char const *path) {
  return read_txt_file_len(path, NULL);
}

typedef union v2 {
  struct {
    float x, y;
  };

  float v[2];
} v2;

static const v2 v2_ux = (v2){.x = 1};
static const v2 v2_uy = (v2){.y = 1};
static const v2 v2_zero = (v2){0};
static const v2 v2_one = (v2){1, 1};


inline static v2 v2_max(v2 lhs, v2 rhs) {
  return (v2){max(lhs.x, rhs.x), max(lhs.y, rhs.y)};
}

[[gnu::always_inline]]

inline static v2 v2_min(v2 lhs, v2 rhs) {
  return (v2){min(lhs.x, rhs.x), min(lhs.y, rhs.y)};
}

[[gnu::always_inline]]

inline static v2 v2_add(v2 lhs, v2 rhs) {
  return (v2){lhs.x + rhs.x, lhs.y + rhs.y};
}

[[gnu::always_inline]]

inline static v2 v2_sub(v2 lhs, v2 rhs) {
  return (v2){lhs.x - rhs.x, lhs.y - rhs.y};
}

[[gnu::always_inline]]

inline static v2 v2_mul(v2 lhs, float scalar) {
  return (v2){lhs.x * scalar, lhs.y * scalar};
}

[[gnu::always_inline]]

inline static v2 v2_div(v2 lhs, float scalar) {
  return (v2){lhs.x / scalar, lhs.y / scalar};
}

[[gnu::always_inline]]

inline static float v2_dot(v2 lhs, v2 rhs) {
  return lhs.x * rhs.x + lhs.y * rhs.y;
}

[[gnu::always_inline]]

inline static float v2_len(v2 a) {
  return sqrtf(v2_dot(a, a));
}

[[gnu::always_inline]]

inline static float v2_dist(v2 lhs, v2 rhs) {
  v2 delta = v2_sub(lhs, rhs);
  return v2_len(delta);
}

[[gnu::always_inline]]

inline static v2 v2_norm(v2 v) {
  float len = v2_len(v);
  return (v2){.x = v.x / len, .y = v.y / len};
}

typedef union iv2 {
  struct {
    int x, y;
  };

  int v[2];
} iv2;

static const iv2 iv2_ux = (iv2){.x = 1};
static const iv2 iv2_uy = (iv2){.y = 1};
static const iv2 iv2_zero = (iv2){0};

[[gnu::always_inline]]

inline static iv2 iv2_max(iv2 lhs, iv2 rhs) {
  return (iv2){max(lhs.x, rhs.x), max(lhs.y, rhs.y)};
}

[[gnu::always_inline]]

inline static iv2 iv2_min(iv2 lhs, iv2 rhs) {
  return (iv2){min(lhs.x, rhs.x), min(lhs.y, rhs.y)};
}

[[gnu::always_inline]]

inline static iv2 iv2_add(iv2 lhs, iv2 rhs) {
  return (iv2){lhs.x + rhs.x, lhs.y + rhs.y};
}

[[gnu::always_inline]]

inline static bool iv2_eq(iv2 lhs, iv2 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

[[gnu::always_inline]]

inline static iv2 iv2_sub(iv2 lhs, iv2 rhs) {
  return (iv2){lhs.x - rhs.x, lhs.y - rhs.y};
}

[[gnu::always_inline]]

inline static iv2 iv2_mul(iv2 lhs, int scalar) {
  return (iv2){lhs.x * scalar, lhs.y * scalar};
}

[[gnu::always_inline]]

inline static iv2 iv2_div(iv2 lhs, int scalar) {
  return (iv2){lhs.x / scalar, lhs.y / scalar};
}

[[gnu::always_inline]]

inline static int iv2_dot(iv2 lhs, iv2 rhs) {
  return lhs.x * rhs.x + lhs.y * rhs.y;
}

[[gnu::always_inline]]

inline static float iv2_len(iv2 a) {
  return sqrtf((float)iv2_dot(a, a));
}

[[gnu::always_inline]]

inline static float iv2_dist(iv2 lhs, iv2 rhs) {
  iv2 delta = iv2_sub(lhs, rhs);
  return iv2_len(delta);
}

typedef union v3 {
  struct {
    float x, y, z;
  };

  float v[3];
} v3;

static const v3 v3_ux = (v3){.x = 1};
static const v3 v3_uy = (v3){.y = 1};
static const v3 v3_uz = (v3){.z = 1};
static const v3 v3_zero = (v3){0};
static const v3 v3_one = (v3){1, 1, 1};

[[gnu::always_inline]]

inline static void v3_inc(v3 *lhs, v3 rhs) {
  lhs->x += rhs.x;
  lhs->y += rhs.y;
  lhs->z += rhs.z;
}

[[gnu::always_inline]]

inline static v3 v3_max(v3 lhs, v3 rhs) {
  return (v3){max(lhs.x, rhs.x), max(lhs.y, rhs.y), max(lhs.z, rhs.z)};
}

[[gnu::always_inline]]

inline static v3 v3_min(v3 lhs, v3 rhs) {
  return (v3){min(lhs.x, rhs.x), min(lhs.y, rhs.y), min(lhs.z, rhs.z)};
}

inline static void v3_print(v3 v, FILE *out) {
  fprintf(out, "<%f, %f, %f>", v.x, v.y, v.z);
}

[[gnu::always_inline]]

inline static v3 v3_add(v3 lhs, v3 rhs) {
  return (v3){lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

[[gnu::always_inline]]

inline static v3 v3_neg(v3 v) {
  return (v3){-v.x, -v.y, -v.z};
}

[[gnu::always_inline]]

inline static v3 v3_sub(v3 lhs, v3 rhs) {
  return (v3){lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

[[gnu::always_inline]]

inline static v3 v3_mul(v3 lhs, float scalar) {
  return (v3){lhs.x * scalar, lhs.y * scalar, lhs.z * scalar};
}

[[gnu::always_inline]]

inline static v3 v3_mul_v(v3 lhs, v3 rhs) {
  return (v3){lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

[[gnu::always_inline]]

inline static v3 v3_div(v3 lhs, float scalar) {
  return (v3){lhs.x / scalar, lhs.y / scalar, lhs.z / scalar};
}

[[gnu::always_inline]]

inline static v3 v3_cross(v3 lhs, v3 rhs) {
  // 23 31 12
  return (v3){lhs.y * rhs.z - lhs.z * rhs.y,
               lhs.z * rhs.x - lhs.x * rhs.z,
               lhs.x * rhs.y - lhs.y * rhs.x};
}

[[gnu::always_inline]]

inline static float v3_dot(v3 lhs, v3 rhs) {
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[gnu::always_inline]]

inline static float v3_len(v3 v) {
  return sqrtf(v3_dot(v, v));
}

[[gnu::always_inline]]

inline static float v3_angle(v3 lhs, v3 rhs) {
  return acosf(v3_dot(lhs, rhs) / v3_len(lhs) / v3_len(rhs));
}

[[gnu::always_inline]]

inline static float v3_dist(v3 lhs, v3 rhs) {
  v3 delta = v3_sub(lhs, rhs);
  return v3_len(delta);
}

[[gnu::always_inline]]

inline static v3 v3_normed(v3 v) {
  float len = v3_len(v);
  return (v3){.x = v.x / len, .y = v.y / len, .z = v.z / len};
}

[[gnu::always_inline]]

inline static void v3_norm(v3 *v) {
  *v = v3_normed(*v);
}

[[gnu::always_inline]]

inline static v3 v3_lerp(v3 a, v3 b, float d) {
  return v3_add(a, v3_mul(v3_sub(b, a), d));
}

typedef union v4 {
  struct {
    float x, y, z, w;
  };

  struct {
    float r, g, b, a;
  };

  float v[4];
} v4;

static const v4 v4_ux = (v4){.x = 1};
static const v4 v4_uy = (v4){.y = 1};
static const v4 v4_uz = (v4){.z = 1};
static const v4 v4_uw = (v4){.w = 1};
static const v4 v4_zero = (v4){0};
static const v4 v4_one = (v4){1, 1, 1, 1};

[[gnu::always_inline]]

inline static v4 v4_neg(v4 lhs) {
  return (v4){-lhs.x, -lhs.y, -lhs.z, -lhs.w};
}

[[gnu::always_inline]]

inline static v4 v4_add(v4 lhs, v4 rhs) {
  return (v4){lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
}

[[gnu::always_inline]]

inline static v4 v4_sub(v4 lhs, v4 rhs) {
  return (v4){lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
}

[[gnu::always_inline]]

inline static v4 v4_mul(v4 lhs, float scalar) {
  return (v4){lhs.x * scalar, lhs.y * scalar, lhs.z * scalar, lhs.w * scalar};
}

[[gnu::always_inline]]

inline static v4 v4_div(v4 lhs, float scalar) {
  return (v4){lhs.x / scalar, lhs.y / scalar, lhs.z / scalar, lhs.w / scalar};
}

[[gnu::always_inline]]

inline static float v4_dot(v4 lhs, v4 rhs) {
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

[[gnu::always_inline]]

inline static float v4_len(v4 v) {
  return sqrtf(v4_dot(v, v));
}

[[gnu::always_inline]]

inline static v4 v4_lerp(v4 a, v4 b, float d) {
  return v4_add(a, v4_mul(v4_sub(b, a), d));
}

typedef union m4 {
  float e[16];
  float v[4][4];
  v4 r[4];
  struct {
    float
      _00, _01, _02, _03,
      _10, _11, _12, _13,
      _20, _21, _22, _23,
      _30, _31, _32, _33;
  };
} m4;

static void m4_print(m4 m) {
  printf("%f %f %f %f\n", m._00, m._01, m._02, m._03);
  printf("%f %f %f %f\n", m._10, m._11, m._12, m._13);
  printf("%f %f %f %f\n", m._20, m._21, m._22, m._23);
  printf("%f %f %f %f\n", m._30, m._31, m._32, m._33);
}

static const m4 m4_ident = (m4){.r = {v4_ux, v4_uy, v4_uz, v4_uw}};

[[gnu::always_inline]]

inline static v4 m4_col(m4 *m, int col) {
  return (v4){m->v[0][col], m->v[1][col], m->v[2][col], m->v[3][col]};
}

[[gnu::always_inline]]

inline static m4 m4_tpose(m4 orig) {
  m4 out;
  out.r[0] = m4_col(&orig, 0);
  out.r[1] = m4_col(&orig, 1);
  out.r[2] = m4_col(&orig, 2);
  out.r[3] = m4_col(&orig, 3);
  return out;
}

[[gnu::always_inline]]

inline static m4 m4_trans(float x, float y, float z) {
  m4 out = m4_ident;
  out.r[3] = (v4){x, y, z, 1};
  return out;
}

[[gnu::always_inline]]
inline static v4 v4_mul_m(v4 v, m4 mat) {
  return (v4){
    v.x * mat._00 + v.y * mat._10 + v.z * mat._20 + mat._30,
    v.x * mat._01 + v.y * mat._11 + v.z * mat._21 + mat._31,
    v.x * mat._02 + v.y * mat._12 + v.z * mat._22 + mat._32,
    v.x * mat._03 + v.y * mat._13 + v.z * mat._23 + mat._33};
}

[[gnu::always_inline]]

inline static m4 m4_trans_v(v3 p) {
  return m4_trans(p.x, p.y, p.z);
}

[[gnu::always_inline]]

inline static m4 m4_scale(float x, float y, float z) {
  m4 out = m4_ident;
  out.r[0].x = x;
  out.r[1].y = y;
  out.r[2].z = z;
  return out;
}

[[gnu::always_inline]]

inline static m4 m4_scale_v(v3 scale) {
  return m4_scale(scale.x, scale.y, scale.z);
}

static m4 m4_mul(m4 lhs, m4 rhs) {
  m4 out;
  rhs = m4_tpose(rhs);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      out.v[i][j] = v4_dot(lhs.r[i], rhs.r[j]);
    }
  }

  return out;
}

static m4 m4_chg_axis(v3 axis, int ax_num) {
  v3 t = v3_normed(axis);
  int min = 0;
  if (fabsf(t.v[min]) > fabsf(t.v[1])) min = 1;
  if (fabsf(t.v[min]) > fabsf(t.v[2])) min = 2;

  v3 m = {0};
  m.v[min] = 1;

  v3 f = v3_normed(v3_cross(t, m));
  v3 s = v3_normed(v3_cross(t, f));

  switch (ax_num) {
    case 0:
      return (m4){
        .e = {
          t.x, t.y, t.z, 0,
          f.x, f.y, f.z, 0,
          s.x, s.y, s.z, 0,
          0, 0, 0, 1
        }
      };
    case 1:
      return (m4){
        .e = {
          s.x, s.y, s.z, 0,
          t.x, t.y, t.z, 0,
          f.x, f.y, f.z, 0,
          0, 0, 0, 1
        }
      };
    case 2:
      return (m4){
        .e = {
          f.x, f.y, f.z, 0,
          s.x, s.y, s.z, 0,
          t.x, t.y, t.z, 0,
          0, 0, 0, 1
        }
      };
    default:throwf("m4_chg_axis: invalid axis number!");
  }
}

static m4 m4_look(v3 pos, v3 dir, v3 up) {
  v3 f = v3_normed(dir);
  v3 s = v3_normed(v3_cross(f, up));
  v3 u = v3_cross(s, f);

  m4 out = {0};

  out.v[0][0] = s.v[0];
  out.v[0][1] = u.v[0];
  out.v[0][2] = -f.v[0];
  out.v[1][0] = s.v[1];
  out.v[1][1] = u.v[1];
  out.v[1][2] = -f.v[1];
  out.v[2][0] = s.v[2];
  out.v[2][1] = u.v[2];
  out.v[2][2] = -f.v[2];
  out.v[3][0] = -v3_dot(s, pos);
  out.v[3][1] = -v3_dot(u, pos);
  out.v[3][2] = v3_dot(f, pos);
  out.v[0][3] = out.v[1][3] = out.v[2][3] = 0.0f;
  out.v[3][3] = 1.0f;

  return out;
}

static m4 m4_persp(float fovy, float aspect, float z_near, float z_far) {
  m4 out = {0};

  float f = 1.f / tanf(fovy * 0.5f);
  float fn = 1.f / (z_near - z_far);

  out.v[0][0] = f / aspect;
  out.v[1][1] = f;
  out.v[2][2] = (z_near + z_far) * fn;
  out.v[2][3] = -1.0f;
  out.v[3][2] = 2.0f * z_near * z_far * fn;

  return out;
}

static m4
m4_ortho(float left, float right, float bottom, float top, float z_near,
         float z_far) {
  m4 out = m4_ident;

  float inv_rl = 1.0f / (right - left);
  float inv_tb = 1.0f / (top - bottom);
  float inv_fn = 1.0f / (z_far - z_near);

  out.r[0].x = 2 * inv_rl;
  out.r[1].y = 2 * inv_tb;
  out.r[2].z = -2 * inv_fn;

  out.r[3].x = -(right + left) * inv_rl;
  out.r[3].y = -(top + bottom) * inv_tb;
  out.r[3].z = -(z_far + z_near) * inv_fn;

  return out;
}

static m4 m4_rot_y(float rad) {
  float c = cosf(rad), s = sinf(rad);
  m4 out = m4_ident;
  out.r[0].x = c;
  out.r[0].z = -s;
  out.r[2].x = s;
  out.r[2].z = c;

  return out;
}

[[gnu::always_inline]]

inline static float lerp(float start, float end, float delta) {
  return start + (end - start) * delta;
}

[[gnu::always_inline]]

inline static float clamp(float val, float least, float most) {
  return fminf(fmaxf(val, least), most);
}

[[gnu::always_inline]]

inline static float rad(float deg) {
  return deg * 3.1415926f / 180.f;
}

static uint32_t iv2_hash(void *key) {
  return hash_murmur3(key, sizeof(iv2));
}

static bool iv2_peq(void *_lhs, void *_rhs) {
  return memcmp(_lhs, _rhs, sizeof(iv2)) == 0;
}

static int idx_wrap(int size, int index) {
  if (index < 0) index += size;
  index %= size;
  return index;
}

static float rad_wrap(float x) {
  x = fmodf(x + M_PIF, M_PIF * 2);
  if (x < 0)
    x += M_PIF * 2;
  return x - M_PIF;
}

static uint32_t str_hash(void *key) {
  char const **str = key;
  return hash_murmur3(*str, strlen(*str));
}

static bool str_eq(void *_lhs, void *_rhs) {
  char const **lhs = _lhs, **rhs = _rhs;
  return strcmp(*lhs, *rhs) == 0;
}

[[gnu::always_inline]]
inline float rndf(float min, float max) {
  float d = (float)rand() / (float)RAND_MAX;
  return min + d * (max - min);
}

[[gnu::always_inline]]
inline int rndi(int min, int max) {
  int64_t mi = min, ma = max;
  if (mi == ma) return (int)mi;
  return (int)(mi + rand() % (ma - mi));
}

[[gnu::always_inline]]
inline static v4 v4q_slerp(v4 from, v4 to, float t) {
  v4 q1, q2;
  float cosTheta, sinTheta, angle;

  cosTheta = v4_dot(from, to);
  q1 = from;

  if (fabsf(cosTheta) >= 1.0f) {
    return q1;
  }

  if (cosTheta < 0.0f) {
    q1 = v4_neg(q1);
    cosTheta = -cosTheta;
  }

  sinTheta = sqrtf(1.0f - cosTheta * cosTheta);

  /* LERP to avoid zero division */
  if (fabsf(sinTheta) < 0.001f) {
    return v4_lerp(from, to, t);
  }

  /* SLERP */
  angle = acosf(cosTheta);
  q1 = v4_mul(q1, sinf((1.0f - t) * angle));
  q2 = v4_mul(to, sinf(t * angle));

  q1 = v4_add(q1, q2);
  return v4_mul(q1, 1.f / sinTheta);
}

[[gnu::always_inline]]
inline static m4 v4q_to_mat(v4 q) {
  m4 o = {0};

  float q0 = q.w;
  float q1 = q.x;
  float q2 = q.y;
  float q3 = q.z;

  // First row of the rotation matrix
  o._00 = 2 * (q0 * q0 + q1 * q1) - 1;
  o._01 = 2 * (q1 * q2 - q0 * q3);
  o._02 = 2 * (q1 * q3 + q0 * q2);

  // Second row of the rotation matrix
  o._10 = 2 * (q1 * q2 + q0 * q3);
  o._11 = 2 * (q0 * q0 + q2 * q2) - 1;
  o._12 = 2 * (q2 * q3 - q0 * q1);

  // Third row of the rotation matrix
  o._20 = 2 * (q1 * q3 - q0 * q2);
  o._21 = 2 * (q2 * q3 + q0 * q1);
  o._22 = 2 * (q0 * q0 + q3 * q3) - 1;

  o._33 = 1.f;

  return m4_tpose(o);
}