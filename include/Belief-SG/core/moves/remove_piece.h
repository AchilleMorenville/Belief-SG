#ifndef BELIEF_SG_CORE_MOVES_REMOVE_PIECE_H
#define BELIEF_SG_CORE_MOVES_REMOVE_PIECE_H

#include "Belief-SG/core/move.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/position.h"

namespace belief_sg {

class RemovePiece : public Move {
public:
    RemovePiece() = default;
    explicit RemovePiece(const Position& from);

    [[nodiscard]] std::vector<ProbTransition> apply(const State& state) const override;
    void apply_inplace(State& state, std::mt19937& generator) const override;

    [[nodiscard]] std::unique_ptr<Move> clone() const override;
private:
    Position from_;

    [[nodiscard]] bool is_equals(const Move& other) const override;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_MOVES_REMOVE_PIECE_H
