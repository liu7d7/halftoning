#pragma once

#include <assert.h>
#include "typedefs.h"
#include "arr.h"
#include "err.h"

typedef struct lmap {
  void *keys;
  void *vals;
  size_t *idx;
  size_t cap, count, key_size, val_size;
  float load_factor;

  bool (*eq)(void *lhs, void *rhs);

  uint32_t (*hash)(void *key);
} lmap;

void *lmap_add(lmap *l, void *key, void *val);

void *lmap_at(lmap *l, void *key);

bool lmap_has(lmap *l, void *key);

lmap
lmap_new(size_t initial_size, size_t key_size, size_t val_size,
         float load_factor,
         bool(*eq)(void *, void *), uint32_t(*hash)(void *));