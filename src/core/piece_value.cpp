#include "Belief-SG/core/piece_value.h"

#include <vector>
#include <algorithm>

#include "Belief-SG/core/piece_attribute.h"

namespace belief_sg {

PieceValue::PieceValue(const std::vector<PieceAttribute>& attributes) : attributes_(attributes) {
    std::ranges::sort(attributes_, std::ranges::less(), &PieceAttribute::name);
    const auto [first, last] = std::ranges::unique(attributes_, std::ranges::equal_to(), &PieceAttribute::name);
    attributes_.erase(first, last);
}

const PieceAttribute& PieceValue::get_attribute(const std::string& name) const {
    auto iter = std::ranges::find_if(attributes_, [&name](const PieceAttribute& attribute) {
        return attribute.name() == name;
    });
    if (iter == attributes_.end()) {
        throw std::runtime_error("Attribute " + name + " not found");
    }
    return *iter;
}

const std::vector<PieceAttribute>& PieceValue::get_attributes() const {
    return attributes_;
}

std::string PieceValue::to_string() const {
    std::string s = "{";
    for (const PieceAttribute& attribute : attributes_) {
        s += attribute.to_string() + " ";
    }
    s.pop_back();
    s += "}";
    return s;
}

}  // namespace belief_sg
