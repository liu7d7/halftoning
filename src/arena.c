#include "arena.h"

arena arena_new(size_t size) {
  return (arena){
    .base = malloc(size),
    .head = 0,
    .size = size,
    .max = 0
  };
}

void *arena_get(arena *a, size_t size) {
  void *ptr = a->base + a->head;
  a->head += size;
  a->max = max(a->max, a->head);
  if (a->head > a->size) throwf("arena: tried to allocate more than max!");
  return ptr;
}

void arena_reset(arena *a) {
  a->head = 0;
}
