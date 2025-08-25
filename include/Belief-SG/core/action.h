#ifndef BELIEF_SG_CORE_ACTION_H
#define BELIEF_SG_CORE_ACTION_H

#include <memory>
#include <vector>

#include "Belief-SG/core/prob_transition.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/move.h"

namespace belief_sg {

class Action {
public:
    Action() = default;
    explicit Action(std::vector<std::unique_ptr<Move>> moves);

    Action(const Action& other);
    Action& operator=(const Action& other);

    ~Action() = default;

    [[nodiscard]] bool operator==(const Action& other) const;

    [[nodiscard]] std::vector<ProbTransition> apply(const State& state) const;
    void apply_inplace(State& state, std::mt19937& generator) const;

private:
    std::vector<std::unique_ptr<Move>> moves_;
};

struct ProbAction {
    Action action;
    double probability{};
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_ACTION_H
