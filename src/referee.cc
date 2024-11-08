#include "referee.h"

#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <random>

#include "game.h"
#include "agent.h"

Referee::Referee() {}

Referee::~Referee() {}

void Referee::SetGame(std::shared_ptr<Game> game) {
  game_ = game;
  world_state_ = game_->InitialState({0, 1});
  private_states_.push_back(game_->InitialState({0}));
  private_states_.push_back(game_->InitialState({1}));
}

void Referee::AddAgent(int player, std::unique_ptr<Agent> agent) {
  agent->SetGame(game_);
  agent->SetPlayer(player);
  agents_[player] = std::move(agent);
}

void Referee::PlayGame() {

  std::mt19937 generator(std::random_device{}());

  int step = 0;
  while (!game_->IsTerminal(world_state_)) {
    std::cout << "Step : " << step << std::endl << std::endl;

    int current_player = world_state_.GetCurrentPlayer();
    std::cout << "Current Player : " << current_player << std::endl << std::endl;

    Action action = agents_[current_player]->Act(private_states_[current_player]);
    if (action == -1) {
      std::cout << "Player " << current_player << " has no legal actions." << std::endl;
      break;
    }
    std::cout << "Player " << current_player << " chose action " << action << std::endl << std::endl;

    std::vector<StochasticAction> legal_stochastic_actions = game_->LegalActions(world_state_, current_player);
    bool found = false;
    for (const auto& legal_stochastic_action : legal_stochastic_actions) {
      if (legal_stochastic_action.action == action) {
        found = true;
        break;
      }
    }
    if (!found) {
      std::cout << "Action is not legal" << std::endl;
      break;
    }

    std::vector<StochasticTransition> world_transitions = game_->ApplyAction(world_state_, action);
    std::uniform_int_distribution<std::size_t> distribution(0, world_transitions.size() - 1);
    world_state_ = world_transitions[distribution(generator)].next_state;

    game_->print(world_state_);

    std::vector<bool> found_consistent(private_states_.size(), false);
    for (int i = 0; i < private_states_.size(); i++) {
      std::vector<StochasticTransition> private_transitions = game_->ApplyAction(private_states_[i], action);
      for (const auto& private_transition : private_transitions) {
        if (IsConsistent(private_transition.next_state, world_state_)) {
          private_states_[i] = private_transition.next_state;
          found_consistent[i] = true;
          break;
        }
      }
    }

    if (!found_consistent[0]) {
      std::cout << "Player " << 0 << " state is inconsistent" << std::endl;
      break;
    }
    if (!found_consistent[1]) {
      std::cout << "Player " << 1 << " state is inconsistent" << std::endl;
      break;
    }

    std::cout << std::endl;

    if (game_->IsTerminal(world_state_)) {
      std::cout << "Terminal" << std::endl;
      if (game_->Returns(world_state_) != game_->Returns(private_states_[0])) {
        std::cout << "Player 0 returns are different" << std::endl;
        break;
      }
      if (game_->Returns(world_state_) != game_->Returns(private_states_[1])) {
        std::cout << "Player 1 returns are different" << std::endl;
        break;
      }
      std::cout << "Player 0 returns : " << game_->Returns(world_state_)[0] << std::endl;
      std::cout << "Player 1 returns : " << game_->Returns(world_state_)[1] << std::endl;
    }

    step++;
  }
}

bool Referee::IsConsistent(const BeliefState& private_state, const BeliefState& world_state) {
  for (const auto& [tile_id, tile] : world_state.GetTiles()) {
    if (tile.GetPieceIds().size() != private_state.GetTile(tile_id).GetPieceIds().size()) {
      return false;
    }
    for (int i = 0; i < tile.GetPieceIds().size(); i++) {
      for (const auto& [type, prob] : world_state.GetPiece(tile.GetPieceId(i)).GetProbabilities()) {
        if (std::abs(prob - 1.0) >= 1e-6) {
          continue;
        } else {
          int piece_id = private_state.GetTile(tile_id).GetPieceId(i);
          if (std::abs(private_state.GetPiece(piece_id).GetProbability(type)) < 1e-6) {
            return false;
          }
        }
      }
    }
  }
  return true;
}
