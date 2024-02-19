#pragma once

#include <string.h>
#include <stdint.h>
#include "typedefs.h"

typedef struct dyn_arr_metadata dyn_arr_metadata;

struct dyn_arr_metadata {
  size_t len;
  size_t count;
  size_t elem_size;
};

typedef uint8_t byte;

dyn_arr_metadata* internal_dyn_arr_get_metadata(byte* memory);

byte* internal_dyn_arr_new(size_t len, size_t elem_size);

byte* internal_dyn_arr_base_ptr(byte* memory);

#define dyn_arr(type, initial_size) (type*) internal_dyn_arr_new(initial_size, sizeof(type))

void internal_dyn_arr_add(byte** memory, byte* item, size_t size);

#define dyn_arr_add(this, ...) internal_dyn_arr_add((byte**) &(this), (byte*) (__VA_ARGS__), sizeof(*(__VA_ARGS__)))
#define dyn_arr_add_unsafe(this, ...) internal_dyn_arr_add((byte**) &(this), (byte*) (__VA_ARGS__), internal_dyn_arr_get_metadata((byte*) this)->elem_size)
#define dyn_arr_add_i(this, type, ...) internal_dyn_arr_add((byte**) &(this), (byte*) &(type) {__VA_ARGS__}, sizeof(type))

#define dyn_arr_len(this) internal_dyn_arr_get_metadata((byte*) this)->count

bool internal_dyn_arr_has(byte* memory, byte* element, size_t elem_size);

#define dyn_arr_has(this, element) internal_dyn_arr_has((byte*) this, (byte*) &element, sizeof(element))
#define dyn_arr_has_i(this, type, ...) internal_dyn_arr_has((byte*) this, (byte*) &(type) {__VA_ARGS__}, sizeof(type))

#define dyn_arr_last(this) &(this)[dyn_arr_count(this) - 1]

#define dyn_arr_count(this) internal_dyn_arr_get_metadata((byte*) this)->count

void internal_dyn_arr_del(byte* memory);

#define dyn_arr_del(this) internal_dyn_arr_del((byte*) this)

void* internal_dyn_arr_at(byte* memory, size_t n);

#define dyn_arr_at(this, n) internal_dyn_arr_at((byte*) this, n)