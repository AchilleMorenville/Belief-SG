#include "Belief-SG/agents/random_agent.h"

#include <iostream>

namespace belief_sg {

RandomAgent::RandomAgent() : generator_(std::random_device()()) {}

void RandomAgent::set_game(std::shared_ptr<Game> game) {
  game_ = std::move(game);
}

void RandomAgent::set_player(PlayerId player) {
  player_ = player;
}

Action RandomAgent::act(const State& private_state, const State& public_state) {
  std::vector<ProbAction> actions = game_->legal_actions(private_state, player_);

  std::uniform_int_distribution<int> dist(0, actions.size() - 1);
  return actions[dist(generator_)].action;
}

}  // namespace belief_sg
