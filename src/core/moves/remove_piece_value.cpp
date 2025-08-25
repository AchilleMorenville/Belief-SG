#include "Belief-SG/core/moves/remove_piece_value.h"

#include <vector>

#include "Belief-SG/core/piece_value.h"
#include "Belief-SG/core/prob_transition.h"
#include "Belief-SG/core/state.h"

namespace belief_sg {

RemovePieceValue::RemovePieceValue(const Position& from, const PieceValue& value) : from_(from), value_(value) {}

std::vector<ProbTransition> RemovePieceValue::apply(const State& state) const {
    State new_state(state);
    new_state.remove_piece_value(from_, value_);
    return std::vector<ProbTransition>{ProbTransition({new_state, 1.0})};
}

void RemovePieceValue::apply_inplace(State& state, std::mt19937& generator) const {
    state.remove_piece_value(from_, value_);
}

std::unique_ptr<Move> RemovePieceValue::clone() const {
  return std::make_unique<RemovePieceValue>(from_, value_);
}

bool RemovePieceValue::is_equals(const Move& other) const {
  const auto& other_remove_piece_value = dynamic_cast<const RemovePieceValue&>(other);
  return from_ == other_remove_piece_value.from_ && value_ == other_remove_piece_value.value_;
}

}  // namespace belief_sg
