#include "src/game.h"

int main() {
  game_t g = game(2304, 1440, "the photographer");
  game_run(&g);
  game_cleanup(&g);
  return 0;
}
