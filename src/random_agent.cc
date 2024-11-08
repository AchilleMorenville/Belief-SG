#include "random_agent.h"

#include <random>

RandomAgent::RandomAgent() : generator_(std::random_device()()) {}

RandomAgent::~RandomAgent() {}

void RandomAgent::SetGame(std::shared_ptr<Game> game) {
  game_ = game;
}

void RandomAgent::SetPlayer(int player) {
  player_ = player;
}

Action RandomAgent::Act(const BeliefState& state) {
  std::vector<StochasticAction> legal_stochastic_actions = game_->LegalActions(state, player_);
  if (legal_stochastic_actions.size() <= 0) {
    return -1;
  }
  std::uniform_int_distribution<int> distribution(0, legal_stochastic_actions.size() - 1);
  return legal_stochastic_actions[distribution(generator_)].action;
}
