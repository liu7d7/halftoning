#pragma once

#include <assert.h>
#include "typedefs.h"
#include "dyn_arr.h"
#include "err.h"

struct entry {
  void* key;
  void* val;
  struct entry* next;
};

struct map {
  struct entry* entries;
  size_t c_entries, n_entries, key_size, val_size;
  float load_factor;

  bool (* eq)(void* lhs, void* rhs);

  size_t (* hash)(void* key);
};

struct entry* internal_map_new_entries(size_t size);

struct map map(size_t initial_size, size_t key_size, size_t val_size, float load_factor, bool(*eq)(void*, void*), size_t(*hash)(void*));

bool internal_map_entry_is_invalid(struct entry* entry);

void internal_map_insert_entry(struct map* d, struct entry* e);

void map_set(struct map* d, void* key, void* val);

void* map_at(struct map* d, void* key);

bool map_has(struct map* d, void* key);