#include "avg.h"

avg_num avg_num_new(int size) {
  return (avg_num){.size = size, .buf = calloc(size + 1, sizeof(float))};
}

void avg_num_add(avg_num *a, float f) {
  a->sum -= a->buf[idx_wrap(a->size + 1, a->head - a->size)];
  a->sum += f;
  a->buf[a->head++] = f;
  a->head %= (a->size + 1);
  a->max = max(a->max, f);
  if (a->cur_size < a->size) a->cur_size++;
}

float avg_num_get(avg_num *a) {
  if (a->cur_size == 0) return 0;
  return a->sum / (float)a->cur_size;
}
