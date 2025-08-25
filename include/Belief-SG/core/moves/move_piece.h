#ifndef BELIEF_SG_CORE_MOVES_MOVE_PIECE_H
#define BELIEF_SG_CORE_MOVES_MOVE_PIECE_H

#include "Belief-SG/core/move.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/position.h"

namespace belief_sg {

class MovePiece : public Move {
public:
    MovePiece() = default;
    MovePiece(const Position& from, const Position& to);
    ~MovePiece() override = default;

    [[nodiscard]] std::vector<ProbTransition> apply(const State& state) const override;
    void apply_inplace(State& state, std::mt19937& generator) const override;

    [[nodiscard]] std::unique_ptr<Move> clone() const override;
private:
    Position from_;
    Position to_;

    [[nodiscard]] bool is_equals(const Move& other) const override;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_MOVES_MOVE_PIECE_H
