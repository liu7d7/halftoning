#include "src/app.h"

int main() {
  app g = app_new(2304, 1440, "monkey");
  app_run(&g);
  app_cleanup(&g);
  return 0;
}
