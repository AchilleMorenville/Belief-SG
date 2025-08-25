#ifndef BELIEF_SG_CORE_MOVES_REMOVE_PIECE_VALUE_H
#define BELIEF_SG_CORE_MOVES_REMOVE_PIECE_VALUE_H

#include <memory>
#include <vector>

#include "Belief-SG/core/move.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/prob_transition.h"
#include "Belief-SG/core/position.h"
#include "Belief-SG/core/piece_value.h"

namespace belief_sg {

class RemovePieceValue : public Move {
public:
    RemovePieceValue() = default;
    explicit RemovePieceValue(const Position& from, const PieceValue& value);
    ~RemovePieceValue() override = default;

    [[nodiscard]] std::vector<ProbTransition> apply(const State& state) const override;
    void apply_inplace(State& state, std::mt19937& generator) const override;

    [[nodiscard]] std::unique_ptr<Move> clone() const override;
private:
    Position from_;
    PieceValue value_;

    [[nodiscard]] bool is_equals(const Move& other) const override;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_MOVES_REMOVE_PIECE_VALUE_H
