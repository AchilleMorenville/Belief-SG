#include "Belief-SG/core/piece_attribute.h"

#include <string>
#include <utility>
#include <vector>

namespace belief_sg {

PieceAttribute::PieceAttribute(std::string name, AttributeValue value) : name_(std::move(name)), value_(std::move(value)) {}

const std::string& PieceAttribute::name() const {
    return name_;
}

std::string PieceAttribute::to_string() const {
    return "{" + name_ + ", " + std::visit([](const auto& val) -> std::string {
        using T = std::decay_t<decltype(val)>;

        if constexpr (std::is_same_v<T, bool>) {
            return val ? "true" : "false";
        } else if constexpr (std::is_same_v<T, double>) {
            return std::to_string(val);  // std::to_string ensures correct double formatting
        } else if constexpr (std::is_same_v<T, int>) {
            return std::to_string(val);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return val;
        } else if constexpr (std::is_same_v<T, std::vector<std::string>> ||
                             std::is_same_v<T, std::vector<int>> ||
                             std::is_same_v<T, std::vector<double>> ||
                             std::is_same_v<T, std::vector<bool>>) {
            std::string result = "{";
            bool first = true;
            for (const auto& elem : val) {
                if (!first) result += ", ";
                if constexpr (std::is_same_v<T, std::vector<bool>>) {
                    result += elem ? "true" : "false";
                } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                    result += elem;
                } else {
                    result += std::to_string(elem);
                }
                first = false;
            }
            result += "}";
            return result;
        }
        return "unknown";
    }, value_) + "}";
}

}  // namespace belief_sg
