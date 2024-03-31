#pragma once

#include <assert.h>
#include "typedefs.h"
#include "arr.h"
#include "err.h"

typedef struct entry {
  size_t key_idx;
  size_t val_idx;
  struct entry *next;
} entry;

typedef struct map {
  void *keys;
  void *vals;
  entry *entries;
  size_t c_entries, n_entries, key_size, val_size;
  float load_factor;

  bool (*eq)(void *lhs, void *rhs);

  size_t (*hash)(void *key);
} map;

entry *internal_map_new_entries(size_t size);

map
map_new(size_t initial_size, size_t key_size, size_t val_size,
        float load_factor,
        bool(*eq)(void *, void *), size_t(*hash)(void *));

bool internal_map_entry_is_invalid(entry *entry);

void internal_map_insert_entry(map *d, entry *e);

void map_add(map *d, void *key, void *val);

void *map_at(map *d, void *key);

bool map_has(map *d, void *key);