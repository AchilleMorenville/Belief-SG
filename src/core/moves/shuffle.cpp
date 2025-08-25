#include "Belief-SG/core/moves/shuffle.h"

#include <numeric>

namespace belief_sg {

Shuffle::Shuffle(const Position& from) : from_(from) {}

std::vector<ProbTransition> Shuffle::apply(const State& state) const {
    State new_state(state);

    // First remove all observers from all pieces
    new_state.hide(from_);

    // Then shuffle the pieces
    new_state.shuffle(from_);

    return std::vector<ProbTransition>{ProbTransition({new_state, 1.0})};
}

void Shuffle::apply_inplace(State& state, std::mt19937& generator) const {
    state.hide(from_);
    state.shuffle(from_);
}

std::unique_ptr<Move> Shuffle::clone() const {
    return std::make_unique<Shuffle>(from_);
}

bool Shuffle::is_equals(const Move& other) const {
    const auto& other_shuffle = dynamic_cast<const Shuffle&>(other);
    return from_ == other_shuffle.from_;
}

}  // namespace belief_sg
