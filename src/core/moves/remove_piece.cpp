#include "Belief-SG/core/moves/remove_piece.h"

#include <vector>
#include <memory>

#include "Belief-SG/core/position.h"
#include "Belief-SG/core/prob_transition.h"
#include "Belief-SG/core/state.h"

namespace belief_sg {

RemovePiece::RemovePiece(const Position& from) : from_(from) {}

std::vector<ProbTransition> RemovePiece::apply(const State& state) const {
    State new_state(state);
    new_state.remove_piece(from_);
    return std::vector<ProbTransition>{ProbTransition({new_state, 1.0})};
}

void RemovePiece::apply_inplace(State& state, std::mt19937& generator) const {
    state.remove_piece(from_);
}

std::unique_ptr<Move> RemovePiece::clone() const {
  return std::make_unique<RemovePiece>(from_);
}

bool RemovePiece::is_equals(const Move& other) const {
  const auto& other_remove_piece = dynamic_cast<const RemovePiece&>(other);
  return from_ == other_remove_piece.from_;
}

}  // namespace belief_sg
