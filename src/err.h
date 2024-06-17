#pragma once

#include <stdio.h>
#include <process.h>
#include <stdarg.h>

#define result_definition(name, T) \
struct name { \
  union {                  \
    T ok;                  \
    char* err; \
  };                       \
                           \
  bool is_ok;\
}; \
 \
static struct name name##_ok(T ok) {         \
  return (struct name) {              \
    .ok = ok,                         \
    .is_ok = true\
  };                                    \
}                                     \
\
static struct name name##_err(char* err) { \
  return (struct name) {              \
    .err = err,                       \
    .is_ok = false\
  };                                    \
}                                                \
                                                 \
static T name##_get(struct name self) {                     \
  if (self.is_ok) {                                 \
    return self.ok;                                             \
  }                                              \
                                                 \
  fprintf((stderr), "%s", self.err);               \
  exit(-1); \
}

[[noreturn]]

static void throwf(char const *error, ...) {
  va_list args;
  va_start(args, error);
  vfprintf(stderr, error, args);
  va_end(args);
  exit(-1);
}