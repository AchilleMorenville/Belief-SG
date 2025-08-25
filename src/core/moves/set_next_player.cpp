#include "Belief-SG/core/moves/set_next_player.h"

#include <vector>

#include "Belief-SG/core/prob_transition.h"
#include "Belief-SG/core/state.h"

namespace belief_sg {

SetNextPlayer::SetNextPlayer(PlayerId next_player_id) : next_player_id_(next_player_id) {}

std::vector<ProbTransition> SetNextPlayer::apply(const State& state) const {
    State new_state(state);
    new_state.set_current_player(next_player_id_);
    return std::vector<ProbTransition>{ProbTransition({new_state, 1.0})};
}

void SetNextPlayer::apply_inplace(State& state, std::mt19937& generator) const {
    state.set_current_player(next_player_id_);
}

std::unique_ptr<Move> SetNextPlayer::clone() const {
  return std::make_unique<SetNextPlayer>(next_player_id_);
}

bool SetNextPlayer::is_equals(const Move& other) const {
  const auto& other_move_piece = dynamic_cast<const SetNextPlayer&>(other);
  return next_player_id_ == other_move_piece.next_player_id_;
}

}  // namespace belief_sg
