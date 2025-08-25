#include "Belief-SG/core/piece_type.h"

#include <vector>
#include <algorithm>
#include <iterator>
#include <stdexcept>

#include "Belief-SG/core/piece_value.h"

namespace belief_sg {

PieceType::PieceType(const std::vector<PieceValue>& values) : values_(values) {}

int PieceType::size() const {
    return static_cast<int>(values_.size());
}

bool PieceType::contains(const PieceValue& value) const {
    return std::ranges::find(values_, value) != values_.end();
}

int PieceType::value_to_index(const PieceValue& value) const {
    const auto iter = std::ranges::find(values_, value);
    if (iter == values_.end()) {
        throw std::invalid_argument("Value not found in PieceType");
    }
    return static_cast<int>(std::distance(values_.begin(), iter));
}

const PieceValue& PieceType::value_from_index(int index) const {
    return values_[index];
}



}  // namespace belief_sg
