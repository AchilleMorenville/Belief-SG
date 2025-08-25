#ifndef BELIEF_SG_CORE_PIECE_TYPE_H
#define BELIEF_SG_CORE_PIECE_TYPE_H

#include <vector>

#include "Belief-SG/core/piece_value.h"

namespace belief_sg {

class PieceType {
public:
    explicit PieceType(const std::vector<PieceValue>& values);

    [[nodiscard]] int size() const;

    [[nodiscard]] bool contains(const PieceValue& value) const;

    [[nodiscard]] int value_to_index(const PieceValue& value) const;
    [[nodiscard]] const PieceValue& value_from_index(int index) const;
private:
    std::vector<PieceValue> values_;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_PIECE_TYPE_H
