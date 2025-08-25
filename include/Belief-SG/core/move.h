#ifndef BELIEF_SG_CORE_MOVE_H
#define BELIEF_SG_CORE_MOVE_H

#include <memory>
#include <vector>

#include "Belief-SG/core/prob_transition.h"
#include "Belief-SG/core/state.h"

namespace belief_sg {

class Move {
public:
    virtual ~Move() = default;

    [[nodiscard]] virtual std::vector<ProbTransition> apply(const State& state) const = 0;
    virtual void apply_inplace(State& state, std::mt19937& generator) const = 0;

    [[nodiscard]] virtual std::unique_ptr<Move> clone() const = 0;
    bool operator==(const Move& other) const;
private:
    [[nodiscard]] virtual bool is_equals(const Move& other) const = 0;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_MOVE_H
