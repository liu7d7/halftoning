#include "lmap.h"

size_t *new_entries(size_t size) {
  size_t *it = malloc(sizeof(size_t) * size);
  memset(it, 0xff, sizeof(size_t) * size);

  return it;
}

bool idx_is_invalid(size_t entry) {
  return entry == SIZE_MAX;
}

lmap
lmap_new(size_t initial_size, size_t key_size, size_t val_size,
         float load_factor,
         bool (*eq)(void *, void *), uint32_t (*hash)(void *)) {
  return (lmap){
    .keys = internal_arr_new(initial_size, key_size),
    .vals = internal_arr_new(initial_size, val_size),
    .idx = new_entries(initial_size),
    .cap = initial_size,
    .count = 0,
    .load_factor = load_factor,
    .eq = eq,
    .hash = hash,
    .key_size = key_size,
    .val_size = val_size
  };
}

void lmap_add_idx(lmap *l, size_t idx) {
  void *key = l->keys + idx * l->key_size;
  uint32_t hash = l->hash(key);
  uint32_t entry_idx = hash % l->cap;
//  uint32_t i = 1;
  while (!idx_is_invalid(l->idx[entry_idx]) &&
         !l->eq(key, l->keys + l->idx[entry_idx] * l->key_size)) {
//    entry_idx += i * i;
//    i++;
    entry_idx++;
    entry_idx %= l->cap;
  }

  l->idx[entry_idx] = idx;
}

lmap lmap_resize(lmap *l, size_t new_size) {
  lmap new = {
    .keys = l->keys,
    .vals = l->vals,
    .idx = new_entries(new_size),
    .cap = new_size,
    .count = l->count,
    .load_factor = l->load_factor,
    .eq = l->eq,
    .hash = l->hash,
    .key_size = l->key_size,
    .val_size = l->val_size
  };

  for (size_t i = 0, len = arr_len(l->keys); i < len; i++) {
    lmap_add_idx(&new, i);
  }

  free(l->idx);

  return new;
}

void lmap_add(lmap *l, void *key, void *val) {
  if (l->count > l->cap * l->load_factor) {
    *l = lmap_resize(l, l->cap * 2);
  }

  arr_add(&l->keys, key);
  arr_add(&l->vals, val);

  lmap_add_idx(l, arr_len(l->keys) - 1);
  l->count++;
}

void *lmap_at(lmap *l, void *key) {
  uint32_t hash = l->hash(key);
  uint32_t entry_idx = hash % l->cap;
//  uint32_t i = 1;
  while (!idx_is_invalid(l->idx[entry_idx]) &&
         !l->eq(key, l->keys + l->idx[entry_idx] * l->key_size)) {
//    entry_idx += i * i;
//    i++;
    entry_idx++;
    entry_idx %= l->cap;
  }

  if (idx_is_invalid(l->idx[entry_idx])) return NULL;

  return l->vals + l->idx[entry_idx] * l->val_size;
}

bool lmap_has(lmap *l, void *key) {
  return lmap_at(l, key) != NULL;
}