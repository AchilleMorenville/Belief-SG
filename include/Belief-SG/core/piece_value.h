#ifndef BELIEF_SG_CORE_PIECE_VALUE_H
#define BELIEF_SG_CORE_PIECE_VALUE_H

#include <vector>

#include "Belief-SG/core/piece_attribute.h"

namespace belief_sg {

class PieceValue {
public:
    explicit PieceValue(const std::vector<PieceAttribute>& attributes);
    PieceValue() = default;

    [[nodiscard]] const PieceAttribute& get_attribute(const std::string& name) const;
    [[nodiscard]] const std::vector<PieceAttribute>& get_attributes() const;

    bool operator==(const PieceValue& other) const = default;

    [[nodiscard]] std::string to_string() const;
private:
    std::vector<PieceAttribute> attributes_;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_PIECE_VALUE_H
