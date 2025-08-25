#include "Belief-SG/core/position.h"

#include <stdexcept>

namespace belief_sg {

Position::Position(int cell_id) : cell_id_(cell_id) {}

Position::Position(int cell_id, int stack_id) : cell_id_(cell_id), stack_id_(stack_id) {}

int Position::cell_id() const {
    return cell_id_;
}

bool Position::has_stack_id() const {
    return stack_id_.has_value();
}

int Position::stack_id() const {
    if (!stack_id_.has_value()) {
        throw std::runtime_error("Position does not have a stack id");
    }
    return stack_id_.value();
}

bool Position::operator==(const Position& other) const {
    return cell_id_ == other.cell_id_ && stack_id_ == other.stack_id_;
}

}  // namespace belief_sg
