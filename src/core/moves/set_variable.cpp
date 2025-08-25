#include "Belief-SG/core/moves/set_variable.h"

#include <vector>

#include "Belief-SG/core/prob_transition.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/variable.h"

namespace belief_sg {

SetVariable::SetVariable(Variable variable) : variable_(std::move(variable)) {}

std::vector<ProbTransition> SetVariable::apply(const State& state) const {
    State new_state(state);
    new_state.set_variable(variable_);
    return std::vector<ProbTransition>{ProbTransition({new_state, 1.0})};
}

void SetVariable::apply_inplace(State& state, std::mt19937& generator) const {
    state.set_variable(variable_);
}

std::unique_ptr<Move> SetVariable::clone() const {
  return std::make_unique<SetVariable>(variable_);
}

bool SetVariable::is_equals(const Move& other) const {
  const auto& other_move_piece = dynamic_cast<const SetVariable&>(other);
  return variable_ == other_move_piece.variable_;
}

}  // namespace belief_sg
