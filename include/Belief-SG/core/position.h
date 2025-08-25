#ifndef BELIEF_SG_CORE_POSITION_H
#define BELIEF_SG_CORE_POSITION_H

#include <optional>

namespace belief_sg {

class Position {
public:
    explicit Position(int cell_id);
    Position(int cell_id, int stack_id);
    Position() = default;

    [[nodiscard]] int cell_id() const;
    [[nodiscard]] bool has_stack_id() const;
    [[nodiscard]] int stack_id() const;

    bool operator==(const Position& other) const;
private:
    int cell_id_{};
    std::optional<int> stack_id_;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_POSITION_H
