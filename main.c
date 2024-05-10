#include "src/app.h"
#include "Windows.h"

__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;

int main() {
  the_app = app_new(2304, 1440, "wip");
  app_run(&the_app);
  app_cleanup(&the_app);
  return 0;
}
