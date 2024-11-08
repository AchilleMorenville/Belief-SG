#include "referee.h"
#include "game.h"
#include "stratego.h"
#include "random_agent.h"

int main() {
  std::shared_ptr<Game> game = std::make_shared<StrategoGame>();
  Referee referee;
  referee.SetGame(game);
  referee.AddAgent(0, std::make_unique<RandomAgent>());
  referee.AddAgent(1, std::make_unique<RandomAgent>());
  referee.PlayGame();
  return 0;
}