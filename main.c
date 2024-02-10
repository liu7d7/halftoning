#include "src/game.h"

int main() {
  struct game g = game(2304, 1440, "l'imprimante");
  game_run(&g);
  game_cleanup(&g);
  return 0;
}
