#include "Belief-SG/core/moves/set_observers.h"

#include "Belief-SG/core/piece_value.h"
#include "Belief-SG/core/position.h"
#include "Belief-SG/core/prob_transition.h"

#include <cstddef>
#include <random>
#include <vector>

namespace belief_sg {

SetObservers::SetObservers(const Position& from, const std::vector<PlayerId>& observers) : from_(from), observers_(observers) {}

std::vector<ProbTransition> SetObservers::apply(const State& state) const {

    State copy_state(state);
    copy_state.hide(from_);
    bool is_seen = copy_state.add_observers(from_, observers_);
    if (!is_seen) {
        return std::vector<ProbTransition>{ProbTransition({copy_state, 1.0})};
    }

    std::vector<ProbTransition> transitions;
    if (from_.has_stack_id()) {
        const Piece& piece = copy_state.get_piece_at(from_);
        for (const PieceValue& value : piece.values) {
            State new_state(copy_state);
            new_state.assign_piece_value(from_, value);
            transitions.push_back(ProbTransition({new_state, piece.probability(value)}));
        }
    } else {
        transitions.push_back(ProbTransition({copy_state, 1.0}));

        int n_pieces = static_cast<int>(copy_state.get_pieces_at(from_).size());
        for (int i = 0; i < n_pieces; i++) {

            std::vector<ProbTransition> new_transitions;
            for (const ProbTransition& transition : transitions) {
                const Piece& piece = transition.state.get_piece_at(Position(from_.cell_id(), i));
                for (const PieceValue& value : piece.values) {
                    State new_state(transition.state);
                    new_state.assign_piece_value(Position(from_.cell_id(), i), value);
                    new_transitions.push_back(ProbTransition({new_state, transition.probability * piece.probability(value)}));
                }
            }
            transitions = new_transitions;
        }
    }
    return transitions;
}

void SetObservers::apply_inplace(State& state, std::mt19937& generator) const {
    state.hide(from_);
    bool is_seen = state.add_observers(from_, observers_);
    if (!is_seen) {
      return;
    }

    if (from_.has_stack_id()) {
        const Piece& piece = state.get_piece_at(from_);
        std::uniform_int_distribution<std::size_t> dist(0, piece.values.size()-1);
        state.assign_piece_value(from_, piece.values[dist(generator)]);
    } else {
        int n_pieces = static_cast<int>(state.get_pieces_at(from_).size());
        for (int i = 0; i < n_pieces; i++) {
            const Piece& piece = state.get_piece_at(Position(from_.cell_id(), i));
            std::uniform_int_distribution<std::size_t> dist(0, piece.values.size()-1);
            state.assign_piece_value(Position(from_.cell_id(), i), piece.values[dist(generator)]);
        }
    }
}

std::unique_ptr<Move> SetObservers::clone() const {
  return std::make_unique<SetObservers>(from_, observers_);
}

bool SetObservers::is_equals(const Move& other) const {
  const auto& other_set_observers = dynamic_cast<const SetObservers&>(other);
  return from_ == other_set_observers.from_ && observers_ == other_set_observers.observers_;
}

}  // namespace belief_sg
