#include "Belief-SG/core/moves/move_piece.h"

#include <vector>

#include "Belief-SG/core/position.h"
#include "Belief-SG/core/prob_transition.h"
#include "Belief-SG/core/state.h"

namespace belief_sg {

MovePiece::MovePiece(const Position& from, const Position& to) : from_(from), to_(to) {}

std::vector<ProbTransition> MovePiece::apply(const State& state) const {
    State new_state(state);
    new_state.move_piece(from_, to_);
    return std::vector<ProbTransition>{ProbTransition({new_state, 1.0})};
}

void MovePiece::apply_inplace(State& state, std::mt19937& generator) const {
    state.move_piece(from_, to_);
}

std::unique_ptr<Move> MovePiece::clone() const {
  return std::make_unique<MovePiece>(from_, to_);
}

bool MovePiece::is_equals(const Move& other) const {
  const auto& other_move_piece = dynamic_cast<const MovePiece&>(other);
  return from_ == other_move_piece.from_ && to_ == other_move_piece.to_;
}

}  // namespace belief_sg
