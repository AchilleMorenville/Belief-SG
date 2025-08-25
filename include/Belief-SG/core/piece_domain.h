#ifndef BELIEF_SG_CORE_PIECE_DOMAIN_H
#define BELIEF_SG_CORE_PIECE_DOMAIN_H

#include <memory>
#include <vector>

#include "Belief-SG/core/piece_type.h"

namespace belief_sg {

class PieceDomain {
public:
    explicit PieceDomain(const std::shared_ptr<const PieceType>& type);
    PieceDomain() = default;

    [[nodiscard]] bool is_fixed() const;
    [[nodiscard]] bool is_value(int value) const;

    [[nodiscard]] int value() const;

    [[nodiscard]] bool contains(int value) const;

    [[nodiscard]] bool is_subset_of(const PieceDomain& other) const;

    bool assign(int value);
    bool remove(int value);

    void reset();

    [[nodiscard]] const std::vector<bool>& domain() const;

    void set_marginal(int value, double marginal);
    [[nodiscard]] double marginal(int value) const;
private:
    int domain_size_{};
    std::vector<bool> domain_;
    std::vector<double> marginals_;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_PIECE_DOMAIN_H
