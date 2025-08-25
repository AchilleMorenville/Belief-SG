#include "Belief-SG/core/piece_domain.h"

#include <memory>
#include <stdexcept>
#include <algorithm>
#include <vector>

#include "Belief-SG/core/piece_type.h"

namespace belief_sg {

PieceDomain::PieceDomain(const std::shared_ptr<const PieceType>& type) : domain_size_(type->size()), domain_(domain_size_, true), marginals_(domain_size_, 1.0 / type->size()) {}

bool PieceDomain::is_fixed() const {
    return domain_size_ == 1;
}

bool PieceDomain::is_value(int value) const {
    return domain_[value] && domain_size_ == 1;
}

int PieceDomain::value() const {
    if (!is_fixed()) {
        throw std::runtime_error("Cannot get value of a non-fixed domain");
    }
    return static_cast<int>(std::ranges::find(domain_, true) - domain_.begin());
}

bool PieceDomain::contains(int value) const {
    return domain_[value];
}

bool PieceDomain::is_subset_of(const PieceDomain& other) const {
    for (int i = 0; i < domain_.size(); i++) {
        if (domain_[i] && !other.domain_[i]) {
            return false;
        }
    }
    return true;
}

bool PieceDomain::assign(int value) {
    if (value < 0) {
        return false;
    }

    if (is_value(value)) {
        return false;
    }

    if (is_fixed()) {
        throw std::runtime_error("Cannot assign another value to a fixed domain");
    }

    std::fill(domain_.begin(), domain_.end(), false);
    domain_[value] = true;

    std::ranges::fill(marginals_, 0.0);
    marginals_[value] = 1.0;

    domain_size_ = 1;

    return true;
}

bool PieceDomain::remove(int value) {
    if (value < 0 || !domain_[value]) {
        return false;
    }

    if (is_fixed()) {
        throw std::runtime_error("Cannot remove a value from a fixed domain");
    }

    domain_[value] = false;
    domain_size_--;

    marginals_[value] = 0.0;

    return true;
}

void PieceDomain::reset() {
    std::fill(domain_.begin(), domain_.end(), true);
    domain_size_ = static_cast<int>(domain_.size());
    std::ranges::fill(marginals_, 1.0 / domain_size_);
}

const std::vector<bool>& PieceDomain::domain() const {
    return domain_;
}

void PieceDomain::set_marginal(int value, double marginal) {
    marginals_[value] = marginal;
}

double PieceDomain::marginal(int value) const {
    return marginals_[value];
}

}  // namespace belief_sg
