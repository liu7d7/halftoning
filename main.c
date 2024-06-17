#include "src/app.h"
#include "Windows.h"

int main() {
  $ = app_new(2304, 1440, "wip");
  app_run(&$);
  app_cleanup(&$);
  return 0;
}
