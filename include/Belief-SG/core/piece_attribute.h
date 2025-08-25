#ifndef BELIEF_SG_CORE_PIECE_ATTRIBUTE_H
#define BELIEF_SG_CORE_PIECE_ATTRIBUTE_H

#include <variant>
#include <string>

namespace belief_sg {

using AttributeValue = std::variant<int, double, std::string>;

class PieceAttribute {
public:
    PieceAttribute(std::string name, AttributeValue value);
    PieceAttribute() = default;

    [[nodiscard]] const std::string& name() const;

    template <typename T>
    [[nodiscard]] T value() const {
        if (!std::holds_alternative<T>(value_)) {
            throw std::bad_variant_access();
        }
        return std::get<T>(value_);
    }

    bool operator==(const PieceAttribute& other) const = default;

    [[nodiscard]] std::string to_string() const;
private:
    std::string name_;
    AttributeValue value_;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_PIECE_ATTRIBUTE_H
