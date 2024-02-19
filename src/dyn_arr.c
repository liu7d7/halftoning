#include <assert.h>
#include "dyn_arr.h"

dyn_arr_metadata* internal_dyn_arr_get_metadata(byte* memory) {
  return ((dyn_arr_metadata*) (memory - sizeof(dyn_arr_metadata)));
}

byte* internal_dyn_arr_new(size_t len, size_t elem_size) {
  byte* memory = malloc(sizeof(dyn_arr_metadata) + len * elem_size);
  memcpy(memory, &(dyn_arr_metadata) {
    .len = len,
    .count = 0,
    .elem_size = elem_size
  }, sizeof(dyn_arr_metadata));

  return memory + sizeof(dyn_arr_metadata);
}

byte* internal_dyn_arr_base_ptr(byte* memory) {
  return memory - sizeof(dyn_arr_metadata);
}

void internal_dyn_arr_add(byte** memory, byte* item, size_t size) {
  dyn_arr_metadata* data = internal_dyn_arr_get_metadata(*memory);
  if (data->elem_size != size) {
    assert(false && "incompatible element sizes!");
  }

  if (data->len == data->count) {
    data->len *= 2;
    size_t new_size_in_bytes = sizeof(dyn_arr_metadata) + data->len * data->elem_size;
    byte* new_base_ptr = realloc(internal_dyn_arr_base_ptr(*memory), new_size_in_bytes);
    *memory = new_base_ptr + sizeof(dyn_arr_metadata);

    data = internal_dyn_arr_get_metadata(*memory);
  }

  memcpy(*memory + data->count * data->elem_size, item, data->elem_size);
  data->count++;
}

bool internal_dyn_arr_has(byte* memory, byte* element, size_t elem_size) {
  dyn_arr_metadata* data = internal_dyn_arr_get_metadata(memory);

  if (data->elem_size != elem_size) {
    assert(false && "incompatible type");
  }

  for (int i = 0; i < data->count; i++) {
    byte* a = memory + data->elem_size * i;
    if (!memcmp(a, element, data->elem_size)) {
      return true;
    }
  }

  return false;
}

void internal_dyn_arr_del(byte* memory) {
  byte* base = internal_dyn_arr_base_ptr(memory);
  free(base);
}

void* internal_dyn_arr_at(byte* memory, size_t n) {
  dyn_arr_metadata* meta = internal_dyn_arr_get_metadata(memory);
  return memory + meta->elem_size * n;
}
