#include "map.h"

struct entry* internal_map_new_entries(size_t size) {
  static const struct entry invalid_entry = {
    .key_idx = SIZE_MAX,
    .val_idx = SIZE_MAX,
    .next = NULL,
  };

  struct entry* it = malloc(sizeof(struct entry) * size);
  for (int i = 0; i < size; i++) {
    it[i] = invalid_entry;
  }

  return it;
}

bool internal_map_entry_is_invalid(struct entry* entry) {
  return entry->key_idx == SIZE_MAX;
}

void internal_map_insert_entry(struct map* d, struct entry* e) {
  size_t insert_index = d->hash(dyn_arr_at(d->keys, e->key_idx)) % d->c_entries;

  if (internal_map_entry_is_invalid(&d->entries[insert_index])) {
    d->entries[insert_index] = *e;
  } else {
    struct entry* heap = malloc(sizeof(*e));
    memcpy(heap, e, sizeof(*e));

    struct entry* end = &d->entries[insert_index];
    while (end->next) {
      end = end->next;
    }

    end->next = heap;
  }

  d->n_entries++;
}

void map_set(struct map* d, void* key, void* val) {
  if (d->n_entries > d->c_entries * (long double)d->load_factor) {
    struct map new_d = {
      .keys = d->keys,
      .vals = d->vals,
      .entries = internal_map_new_entries(d->c_entries * 2),
      .c_entries = d->c_entries * 2,
      .n_entries = 0,
      .eq = d->eq,
      .hash = d->hash,
      .load_factor = d->load_factor,
      .key_size = d->key_size,
      .val_size = d->val_size
    };

    for (int i = 0; i < d->c_entries; i++) {
      struct entry* e = &d->entries[i];
      if (internal_map_entry_is_invalid(e)) {
        continue;
      }

      if (e->next) {
        struct entry* e1 = e->next;
        while (e1) {
          struct entry to_insert = *e1;
          to_insert.next = NULL;
          internal_map_insert_entry(&new_d, &to_insert);
          struct entry* to_free = e1;
          e1 = e1->next;
          free(to_free);
        }
      }

      e->next = NULL;
      internal_map_insert_entry(&new_d, e);
    }

    map_set(&new_d, key, val);
    free(d->entries);
    *d = new_d;

    return;
  }

  dyn_arr_add_unchecked(d->keys, key);
  dyn_arr_add_unchecked(d->vals, val);

  struct entry e = {
    .key_idx = dyn_arr_count(d->keys) - 1,
    .val_idx = dyn_arr_count(d->vals) - 1,
    .next = NULL
  };

  internal_map_insert_entry(d, &e);
}

void* map_at(struct map* d, void* key) {
  size_t bucket_index = d->hash(key) % d->c_entries;
  if (internal_map_entry_is_invalid(&d->entries[bucket_index])) {
    return NULL;
  }

  struct entry* end = &d->entries[bucket_index];
  while (end && !d->eq(key, dyn_arr_at(d->keys, end->key_idx))) {
    end = end->next;
  }

  if (!end) {
    return NULL;
  }

  return dyn_arr_at(d->vals, end->val_idx);
}

bool map_has(struct map* d, void* key) {
  return map_at(d, key) != NULL;
}

struct map
map(size_t initial_size, size_t key_size, size_t val_size, float load_factor,
    bool (* eq)(void*, void*), size_t (* hash)(void*)) {
  return (struct map) {
    .keys = internal_dyn_arr_new(initial_size, key_size),
    .vals = internal_dyn_arr_new(initial_size, val_size),
    .entries = internal_map_new_entries(initial_size),
    .c_entries = initial_size,
    .n_entries = 0,
    .load_factor = load_factor,
    .eq = eq,
    .hash = hash,
    .key_size = key_size,
    .val_size = val_size
  };
}
