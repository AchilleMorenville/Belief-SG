#ifndef BELIEF_SG_CORE_VARIABLE_H
#define BELIEF_SG_CORE_VARIABLE_H

#include <variant>
#include <string>
#include <vector>

namespace belief_sg {

using VariableValue = std::variant<std::string, int, double, bool,
                                   std::vector<std::string>, std::vector<int>, std::vector<double>, std::vector<bool>>;

class Variable {
public:
    Variable(std::string name, VariableValue value);
    Variable() = default;

    [[nodiscard]] const std::string& name() const;

    template <typename T>
    [[nodiscard]] T value() const {
        if (!std::holds_alternative<T>(value_)) {
            throw std::bad_variant_access();
        }
        return std::get<T>(value_);
    };

    [[nodiscard]] std::string to_string() const;

    bool operator==(const Variable& other) const;
private:
    std::string name_;
    VariableValue value_;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_VARIABLE_H
