#ifndef BELIEF_SG_CORE_MOVES_REVEAL_H
#define BELIEF_SG_CORE_MOVES_REVEAL_H

#include "Belief-SG/core/move.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/position.h"

namespace belief_sg {

class Reveal : public Move {
public:
    Reveal() = default;
    Reveal(const Position& from, const std::vector<PlayerId>& observers);
    ~Reveal() override = default;

    [[nodiscard]] std::vector<ProbTransition> apply(const State& state) const override;
    void apply_inplace(State& state, std::mt19937& generator) const override;

    [[nodiscard]] std::unique_ptr<Move> clone() const override;
private:
    Position from_;
    std::vector<PlayerId> observers_;

    [[nodiscard]] bool is_equals(const Move& other) const override;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_MOVES_REVEAL_H
