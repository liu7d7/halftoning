#pragma once

#include <stddef.h>
#include <malloc.h>
#include "typedefs.h"

typedef struct arena {
  void *base;
  size_t head;
  size_t size;
  size_t max;
} arena;

arena arena_new(size_t size);

void *arena_get(arena *a, size_t size);

void arena_reset(arena *a);