#include "Belief-SG/core/moves/remove_piece_values.h"

namespace belief_sg {

RemovePieceValues::RemovePieceValues(const Position& from, const std::vector<PieceValue>& values) : from_(from), values_(values) {}

std::vector<ProbTransition> RemovePieceValues::apply(const State& state) const {
    State new_state(state);
    new_state.remove_piece_values(from_, values_);
    return std::vector<ProbTransition>{ProbTransition({new_state, 1.0})};
}

void RemovePieceValues::apply_inplace(State& state, std::mt19937& generator) const {
    state.remove_piece_values(from_, values_);
}

std::unique_ptr<Move> RemovePieceValues::clone() const {
    return std::make_unique<RemovePieceValues>(from_, values_);
}

bool RemovePieceValues::is_equals(const Move& other) const {
    const auto& other_remove_piece_values = dynamic_cast<const RemovePieceValues&>(other);
    return from_ == other_remove_piece_values.from_ && values_ == other_remove_piece_values.values_;
}

}  // namespace belief_sg
