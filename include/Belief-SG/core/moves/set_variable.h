#ifndef BELIEF_SG_CORE_MOVES_SET_VARIABLE_H
#define BELIEF_SG_CORE_MOVES_SET_VARIABLE_H

#include <memory>

#include "Belief-SG/core/move.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/prob_transition.h"
#include "Belief-SG/core/variable.h"

namespace belief_sg {

class SetVariable : public Move {
public:
    SetVariable() = default;
    explicit SetVariable(Variable variable);
    ~SetVariable() override = default;

    [[nodiscard]] std::vector<ProbTransition> apply(const State& state) const override;
    void apply_inplace(State& state, std::mt19937& generator) const override;

    [[nodiscard]] std::unique_ptr<Move> clone() const override;
private:
    Variable variable_;

    [[nodiscard]] bool is_equals(const Move& other) const override;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_MOVES_SET_VARIABLE_H
