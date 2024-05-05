#pragma once

#include "typedefs.h"

typedef struct avg_num {
  int head;
  float *buf;
  int size;
  int cur_size;
  float sum;
  float instant;
} avg_num;

avg_num avg_num_new(int size);

void avg_num_add(avg_num *a, float f);

float avg_num_get(avg_num *a);