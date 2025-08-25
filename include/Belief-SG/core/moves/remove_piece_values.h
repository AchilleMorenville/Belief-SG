#ifndef BELIEF_SG_CORE_MOVES_REMOVE_PIECE_VALUES_H
#define BELIEF_SG_CORE_MOVES_REMOVE_PIECE_VALUES_H

#include <memory>
#include <vector>

#include "Belief-SG/core/move.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/prob_transition.h"
#include "Belief-SG/core/position.h"
#include "Belief-SG/core/piece_value.h"

namespace belief_sg {

class RemovePieceValues : public Move {
public:
    RemovePieceValues() = default;
    RemovePieceValues(const Position& from, const std::vector<PieceValue>& values);

    [[nodiscard]] std::vector<ProbTransition> apply(const State& state) const override;
    void apply_inplace(State& state, std::mt19937& generator) const override;

    [[nodiscard]] std::unique_ptr<Move> clone() const override;
private:
    Position from_;
    std::vector<PieceValue> values_;

    [[nodiscard]] bool is_equals(const Move& other) const override;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_MOVES_REMOVE_PIECE_VALUES_H
