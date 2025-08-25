#include "Belief-SG/core/moves/set_next_players.h"

#include <vector>

#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/prob_transition.h"
#include "Belief-SG/core/state.h"

namespace belief_sg {

SetNextPlayers::SetNextPlayers(const std::vector<PlayerId>& next_player_ids) : next_player_ids_(next_player_ids) {}

std::vector<ProbTransition> SetNextPlayers::apply(const State& state) const {
    State new_state(state);
    new_state.set_current_players(next_player_ids_);
    return std::vector<ProbTransition>{ProbTransition({new_state, 1.0})};
}

void SetNextPlayers::apply_inplace(State& state, std::mt19937& generator) const {
    state.set_current_players(next_player_ids_);
}

std::unique_ptr<Move> SetNextPlayers::clone() const {
  return std::make_unique<SetNextPlayers>(next_player_ids_);
}

bool SetNextPlayers::is_equals(const Move& other) const {
  const auto& other_move_piece = dynamic_cast<const SetNextPlayers&>(other);
  return next_player_ids_ == other_move_piece.next_player_ids_;
}

}  // namespace belief_sg
