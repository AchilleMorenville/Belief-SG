#include "Belief-SG/core/action.h"

namespace belief_sg {

Action::Action(std::vector<std::unique_ptr<Move>> moves) : moves_(std::move(moves)) {}

Action::Action(const Action& other) {
    for (const auto& move : other.moves_) {
        moves_.push_back(move->clone());
    }
}

Action& Action::operator=(const Action& other) {
    if (this == &other) {
        return *this;
    }
    moves_.clear();
    for (const auto& move : other.moves_) {
        moves_.push_back(move->clone());
    }
    return *this;
}

bool Action::operator==(const Action& other) const {
    if (moves_.size() != other.moves_.size()) {
        return false;
    }
    for (size_t i = 0; i < moves_.size(); ++i) {
        if (!(*moves_[i] == *other.moves_[i])) {
            return false;
        }
    }
    return true;
}

std::vector<ProbTransition> Action::apply(const State& state) const {
    std::vector<ProbTransition> transitions;
    transitions.push_back(ProbTransition({state, 1.0}));
    for (const auto& move : moves_) {
        std::vector<ProbTransition> new_transitions;
        for (const auto& transition : transitions) {
            for (const auto& new_transition : move->apply(transition.state)) {
                new_transitions.push_back(ProbTransition({new_transition.state, new_transition.probability * transition.probability}));
            }
        }
        transitions = new_transitions;
    }
    return transitions;
}

void Action::apply_inplace(State& state, std::mt19937& generator) const {
    for (const auto& move : moves_) {
        move->apply_inplace(state, generator);
    }
}

}  // namespace belief_sg
