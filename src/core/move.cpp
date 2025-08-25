#include "Belief-SG/core/move.h"

namespace belief_sg {

bool Move::operator==(const Move& other) const {
    return typeid(*this) == typeid(other) && is_equals(other);
}

}  // namespace belief_sg
