#include "src/app.h"

int main() {
  the_app = app_new(2304, 1440, "wip");
  app_run(&the_app);
  app_cleanup(&the_app);
  return 0;
}
