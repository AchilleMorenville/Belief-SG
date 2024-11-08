#ifndef REFEREE_H_
#define REFEREE_H_

#include <memory>
#include <unordered_map>

#include "game.h"
#include "agent.h"

class Referee {
public:
  Referee();
  ~Referee();

  void SetGame(std::shared_ptr<Game> game);

  void AddAgent(int player, std::unique_ptr<Agent> agent);

  void PlayGame();

private:

  bool IsConsistent(const BeliefState& private_state, const BeliefState& world_state);

  std::shared_ptr<Game> game_;
  std::unordered_map<int, std::unique_ptr<Agent>> agents_;
  BeliefState world_state_;
  std::vector<BeliefState> private_states_;
};

#endif  // REFEREE_H_