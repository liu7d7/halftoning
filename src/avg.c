#include "avg.h"

avg_num avg_num_new(int size) {
  return (avg_num){.size = size, .buf = calloc(size + 1, sizeof(float))};
}

int wrap_index(int size, int index) {
  if (index < 0) index += size;
  index %= size;
  return index;
}

void avg_num_add(avg_num *a, float f) {
  a->sum -= a->buf[wrap_index(a->size + 1, a->head - a->size)];
  a->sum += f;
  a->buf[a->head++] = a->instant = f;
  a->head %= (a->size + 1);
  if (a->cur_size < a->size) a->cur_size++;
}

float avg_num_get(avg_num *a) {
  if (a->cur_size == 0) return 0;
  return a->sum / (float)a->cur_size;
}
