#include "Belief-SG/core/state.h"

#include <cstddef>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <algorithm>
#include <iterator>

#include <gecode/int.hh>
#include <gecode/search.hh>

#include "Belief-SG/core/piece_value.h"
#include "Belief-SG/core/position.h"
#include "Belief-SG/core/variable.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/game.h"
#include "Belief-SG/core/point_of_view.h"

namespace belief_sg {

bool Piece::can_be(const PieceValue& value) const {
    return std::ranges::find(values, value) != values.end();
}

bool Piece::can_have(const PieceAttribute& attribute) const {
    return std::ranges::find_if(values, [attribute](const PieceValue& value) {
        return value.get_attribute(attribute.name()) == attribute;
    }) != values.end();
}

bool Piece::can_not_have(const PieceAttribute& attribute) const {
    return std::ranges::find_if(values, [attribute](const PieceValue& value) {
        return value.get_attribute(attribute.name()) != attribute;
    }) != values.end();
}

double Piece::probability(const PieceValue& value) const  {
    auto it = std::ranges::find(values, value);
    if (it == values.end()) {
        return 0.0;
    }
    auto pos = std::distance(values.begin(), it);
    return probs[pos];
}

CollectionModel::CollectionModel(int n_pieces, int n_values) : pieces_(*this, n_pieces, 0, n_values-1), n_pieces_(n_pieces), n_values_(n_values) {
    branch(*this, pieces_, Gecode::INT_VAR_SIZE_MIN(), Gecode::INT_VAL_MIN());
}

CollectionModel::CollectionModel(CollectionModel &s) : Gecode::Space(s), n_pieces_(s.n_pieces_), n_values_(s.n_values_) {
    pieces_.update(*this, s.pieces_);
}

CollectionModel::Space* CollectionModel::copy() {
    return new CollectionModel(*this);
}

int CollectionModel::get_value(int id) const {
    return pieces_[id].val();
}

std::vector<int> CollectionModel::get_values(int id) const {
    std::vector<int> values;
    for (Gecode::IntVarValues i(pieces_[id]); i(); ++i) {
        values.push_back(i.val());
    }
    return values;
}

std::vector<bool> CollectionModel::get_domain(int id) const {
    std::vector<bool> domain(n_values_, false);
    for (Gecode::IntVarValues i(pieces_[id]); i(); ++i) {
        domain[i.val()] = true;
    }
    return domain;
}

std::vector<std::vector<bool>> CollectionModel::get_domains() const {
    std::vector<std::vector<bool>> domains(n_pieces_, std::vector<bool>(n_values_, false));
    for (int id = 0; id < n_pieces_; id++) {
        for (Gecode::IntVarValues i(pieces_[id]); i(); ++i) {
            domains[id][i.val()] = true;
        }
    }
    return domains;
}

void CollectionModel::remove_value(int id, int value) {
    Gecode::rel(*this, pieces_[id], Gecode::IRT_NQ, value);
}

void CollectionModel::assign_value(int id, int value) {
    Gecode::rel(*this, pieces_[id], Gecode::IRT_EQ, value);
}

void CollectionModel::add_counts(std::vector<int> counts) {
    int min_count = *std::min_element(counts.begin(), counts.end());
    int max_count = *std::max_element(counts.begin(), counts.end());

    Gecode::IntVarArgs counts_var(*this, counts.size(), min_count, max_count);
    Gecode::count(*this, pieces_, counts_var, Gecode::IPL_DOM);

    Gecode::IntArgs expected_counts(counts.size());
    for (int i = 0; i < counts.size(); ++i) {
        expected_counts[i] = counts[i];
    }
    for (int i = 0; i < counts.size(); ++i) {
        Gecode::rel(*this, counts_var[i], Gecode::IRT_EQ, expected_counts[i]);
    }
}

void CollectionModel::print() const {
    std::cout << pieces_ << std::endl;
}

BeliefPropagation::BeliefPropagation(int n_pieces, int n_values, const std::vector<int>& counts)
        : n_variables_(n_pieces),
          n_values_(n_values),
          n_constraints_(n_values_),
          counts_(counts),
          variable_messages_(n_variables_, std::vector<std::vector<double>>(n_values_, std::vector<double>(n_constraints_, 0.0))),
          variable_marginals_(n_variables_, std::vector<double>(n_values_, 0.0)),
          constraint_messages_(n_constraints_, std::vector<std::vector<double>>(n_variables_, std::vector<double>(n_values_, 0.0))),
          domains_(n_pieces, std::vector<bool>(n_values, false)) ,
          probabilities_(n_pieces, std::vector<double>(n_values, 1.0 / static_cast<double>(n_values_))) {

    values_.reserve(counts_.size());
    for (int value = 0; value < counts_.size(); value++) {
        values_.push_back(value);
    }
}

void BeliefPropagation::update_probabilities(const std::vector<std::vector<bool>>& domains) {
    if (domains == domains_) {
        return;
    }
    domains_ = domains;

    reset_variables_messages_and_marginals();
    reset_constraints_messages();

    damping_ = 0.5;

    for (int iter = 0; iter < n_iter_; iter++) {
        compute_constraints_messages();
        double change = compute_variables_messages_and_marginals();
        if (change < epsilon_) {
            break;
        }
        damping_ += 0.025;
        damping_ = std::min(damping_, 1.0);
    }

    for (int piece_id = 0; piece_id < n_variables_; piece_id++) {
        for (int value_id = 0; value_id < n_values_; value_id++) {
            probabilities_[piece_id][value_id] = variable_marginals_[piece_id][value_id];
        }
    }
}

void BeliefPropagation::reset_variables_messages_and_marginals() {
    for (int variable_id = 0; variable_id < n_variables_; variable_id++) {
        double domain_size = 0;
        for (bool value_b : domains_[variable_id]) {
            if (value_b) {
                domain_size++;
            }
        }
        double prob = 1.0/domain_size;
        for (int value_id = 0; value_id < n_values_; value_id++) {
            double updated_value = domains_[variable_id][value_id] ? prob : 0.0;
            std::fill(variable_messages_[variable_id][value_id].begin(), variable_messages_[variable_id][value_id].end(), updated_value);
            variable_marginals_[variable_id][value_id] = updated_value;
        }
    }
}

void BeliefPropagation::reset_constraints_messages() {
    for (int constraint_id = 0; constraint_id < n_constraints_; constraint_id++) {
        for (int variable_id = 0; variable_id < n_variables_; variable_id++) {
            std::fill(constraint_messages_[constraint_id][variable_id].begin(), constraint_messages_[constraint_id][variable_id].end(), 0.0);
        }
    }
}

void BeliefPropagation::compute_constraints_messages() {
    for (int constraint_id = 0; constraint_id < n_constraints_; constraint_id++) {
        compute_constraint_messages(constraint_id);
    }
}

void BeliefPropagation::compute_constraint_messages(int constraint_id) {
    // Forward pass
    std::vector<std::vector<double>> prefix(n_variables_, std::vector<double>(counts_[constraint_id]+1, 0.0));
    prefix[0][0] = 1.0;
    for (int variable_id = 0; variable_id < n_variables_-1; variable_id++) {
        for (int value_id = 0; value_id < n_values_; value_id++) {
            if (variable_marginals_[variable_id][value_id] <= 0.0) {
                continue;
            }
            int added_value = static_cast<int>(value_id == values_[constraint_id]);
            for (int j = 0; j < counts_[constraint_id]+1; j++) {
                if (prefix[variable_id][j] > 0.0 && j + added_value <= counts_[constraint_id]) {
                    prefix[variable_id+1][j+added_value] += prefix[variable_id][j] * variable_messages_[variable_id][value_id][constraint_id];
                }
            }
        }
    }

    // Backward pass and message computation
    std::vector<std::vector<double>> suffix(n_variables_, std::vector<double>(counts_[constraint_id]+1, 0.0));
    suffix[n_variables_-1][counts_[constraint_id]] = 1.0;
    for (int variable_id = n_variables_-1; variable_id > 0; variable_id--) {
        for (int value_id = 0; value_id < n_values_; value_id++) {
            if (variable_marginals_[variable_id][value_id] <= 0.0) {
                continue;
            }
            int added_value = static_cast<int>(value_id == values_[constraint_id]);
            double belief = 0.0;
            for (int j = 0; j < counts_[constraint_id]+1; j++) {
                if (j+added_value <= counts_[constraint_id] && suffix[variable_id][j + added_value] > 0.0) {
                    suffix[variable_id-1][j] += suffix[variable_id][j+added_value] * variable_messages_[variable_id][value_id][constraint_id];
                    belief += prefix[variable_id][j] * suffix[variable_id][j+added_value];
                }
            }
            double old_message = constraint_messages_[constraint_id][variable_id][value_id];
            constraint_messages_[constraint_id][variable_id][value_id] = damping_ * belief + (1.0 - damping_) * old_message;
        }
    }
    for (int value_id = 0; value_id < n_values_; value_id++) {
        if (variable_marginals_[0][value_id] <= 0.0) {
            continue;
        }
        int added_value = static_cast<int>(value_id == values_[constraint_id]);
        double old_message = constraint_messages_[constraint_id][0][value_id];
        constraint_messages_[constraint_id][0][value_id] = damping_ * suffix[0][added_value] + (1.0 - damping_) * old_message;
    }

    normalize_constraint_messages(constraint_id);
}

void BeliefPropagation::normalize_constraint_messages(int constraint_id) {
    for (int variable_id = 0; variable_id < n_variables_; variable_id++) {
        double sum = 0.0;
        for (int value_id = 0; value_id < n_values_; value_id++) {
            sum += constraint_messages_[constraint_id][variable_id][value_id];
        }
        for (int value_id = 0; value_id < n_values_; value_id++) {
            constraint_messages_[constraint_id][variable_id][value_id] /= sum;
        }
    }
}

double BeliefPropagation::compute_variables_messages_and_marginals() {
    double max_change = 0.0;
    for (int variable_id = 0; variable_id < n_variables_; variable_id++) {
        max_change = std::max(compute_variable_messages_and_marginals(variable_id), max_change);
    }
    return max_change;
}

double BeliefPropagation::compute_variable_messages_and_marginals(int variable_id) {
    std::vector<double> prev_marginals(variable_marginals_[variable_id]);

    for (int value_id = 0; value_id < n_values_; value_id++) {
        if (variable_marginals_[variable_id][value_id] <= 0.0) {
            continue;
        }
        double marginal = 1.0;
        for (int constraint_id = 0; constraint_id < n_constraints_; constraint_id++) {
            marginal *= constraint_messages_[constraint_id][variable_id][value_id];
        }
        for (int constraint_id = 0; constraint_id < n_constraints_; constraint_id++) {
            double old_message = variable_messages_[variable_id][value_id][constraint_id];
            variable_messages_[variable_id][value_id][constraint_id] = damping_ * marginal / constraint_messages_[constraint_id][variable_id][value_id] + (1.0 - damping_) * old_message;
        }
        variable_marginals_[variable_id][value_id] = marginal;
    }

    normalize_variable_messages(variable_id);
    normalize_variable_marginals(variable_id);

    double max_change = 0.0;
    for (int value_id = 0; value_id < n_values_; value_id++) {
        max_change = std::max(std::abs(prev_marginals[value_id] - variable_marginals_[variable_id][value_id]), max_change);
    }
    return max_change;
}

void BeliefPropagation::normalize_variable_messages(int variable_id) {
    for (int constraint_id = 0; constraint_id < n_constraints_; constraint_id++) {
        double sum = 0.0;
        for (int value_id = 0; value_id < n_values_; value_id++) {
            sum += variable_messages_[variable_id][value_id][constraint_id];
        }
        for (int value_id = 0; value_id < n_values_; value_id++) {
            variable_messages_[variable_id][value_id][constraint_id] /= sum;
        }
    }
}

void BeliefPropagation::normalize_variable_marginals(int variable_id) {
    double sum = 0.0;
    for (int value_id = 0; value_id < n_values_; value_id++) {
        sum += variable_marginals_[variable_id][value_id];
    }
    for (int value_id = 0; value_id < n_values_; value_id++) {
        variable_marginals_[variable_id][value_id] /= sum;
    }
}

CollectionWrapper::CollectionWrapper(std::shared_ptr<const PieceType> ptype, int n_pieces, const std::vector<int>& counts) : type(std::move(ptype)), rbp(n_pieces, type->size(), counts) {
    original_model = std::make_unique<CollectionModel>(n_pieces, type->size());
    original_model->add_counts(counts);
    model = std::make_unique<CollectionModel>(n_pieces, type->size());
    model->add_counts(counts);
    observers = std::vector<std::vector<PlayerId>>(n_pieces, std::vector<PlayerId>());
}

CollectionWrapper::CollectionWrapper(const CollectionWrapper& other)
    : type(other.type),
      original_model(dynamic_cast<CollectionModel*>(other.original_model->clone())),
      model(dynamic_cast<CollectionModel*>(other.model->clone())),
      rbp(other.rbp),
      observers(other.observers) {}

CollectionWrapper& CollectionWrapper::operator=(const CollectionWrapper& other) {
    auto tmp = other;
    swap(tmp);
    return *this;
}

void CollectionWrapper::swap(CollectionWrapper& other) noexcept {
    std::swap(type, other.type);
    std::swap(original_model, other.original_model);
    std::swap(model, other.model);
    std::swap(rbp, other.rbp);
    std::swap(observers, other.observers);
}

std::shared_ptr<const Game> State::game() const {
    return game_;
}

PointOfView State::point_of_view() const {
    return point_of_view_;
}

const std::vector<PlayerId>& State::current_players() const {
    return current_players_;
}

void State::set_current_player(PlayerId player_id) {
    current_players_.clear();
    current_players_.push_back(player_id);
}

void State::set_current_players(const std::vector<PlayerId>& player_ids) {
    current_players_ = player_ids;
}

Piece State::get_piece_at(const Position& position) const {
    PieceIds piece_ids = cells_[position.cell_id()].back();
    if (position.has_stack_id()) {
        piece_ids = cells_[position.cell_id()][position.stack_id()];
    }
    std::vector<int> values = collections_[piece_ids.collection_id].model->get_values(piece_ids.piece_id);
    std::vector<PieceValue> piece_values;
    std::vector<double> piece_probs;
    piece_values.reserve(values.size());
    piece_probs.reserve(values.size());
    for (int value : values) {
        piece_values.push_back(collections_[piece_ids.collection_id].type->value_from_index(value));
        piece_probs.push_back(collections_[piece_ids.collection_id].rbp.get_probability(piece_ids.piece_id, value));
    }
    return {
        .type = collections_[piece_ids.collection_id].type,
        .observers = collections_[piece_ids.collection_id].observers[piece_ids.piece_id],
        .values = piece_values,
        .probs = piece_probs
    };
}

std::vector<Piece> State::get_pieces_at(const Position& position) const {
    std::vector<Piece> pieces;
    pieces.reserve(cells_[position.cell_id()].size());
    for (const PieceIds& piece_ids : cells_[position.cell_id()]) {
        std::vector<int> values = collections_[piece_ids.collection_id].model->get_values(piece_ids.piece_id);
        pieces.push_back({collections_[piece_ids.collection_id].type, collections_[piece_ids.collection_id].observers[piece_ids.piece_id], std::vector<PieceValue>()});
        pieces.back().values.reserve(values.size());
        pieces.back().probs.reserve(values.size());
        for (int value : values) {
            pieces.back().values.push_back(collections_[piece_ids.collection_id].type->value_from_index(value));
            pieces.back().probs.push_back(collections_[piece_ids.collection_id].rbp.get_probability(piece_ids.piece_id, value));
        }
    }
    return pieces;
}

Variable State::variable(const std::string& name) const {
    for (const Variable& variable : variables_) {
        if (variable.name() == name) {
            return variable;
        }
    }
    throw std::runtime_error("Variable " + name + " not found");
}

void State::set_variable(const Variable& variable) {
    for (Variable& var : variables_) {
        if (var.name() == variable.name()) {
            var = variable;
            return;
        }
    }
    variables_.push_back(variable);
}

void State::move_piece(const Position& from, const Position& to) {
    put_piece_id_in_cell(to, pop_piece_id_from_cell(from));
}

void State::remove_piece(const Position& from) {
    pop_piece_id_from_cell(from);
}

void State::remove_piece_value(const Position& from, const PieceValue& value) {
    if (from.has_stack_id()) {
        PieceIds piece_ids = cells_[from.cell_id()][from.stack_id()];
        collections_[piece_ids.collection_id].model->remove_value(
            piece_ids.piece_id,
            collections_[piece_ids.collection_id].type->value_to_index(value)
        );
        Gecode::SpaceStatus status = collections_[piece_ids.collection_id].model->status();
        if (status == Gecode::SS_FAILED) {
            std::cout << this->to_string() << std::endl;
            throw std::runtime_error("Failed to remove piece value (remove_piece_value precise)");
        }
        collections_[piece_ids.collection_id].rbp.update_probabilities(collections_[piece_ids.collection_id].model->get_domains());
        return;
    }
    std::vector<int> updated_collection_ids;
    for (const PieceIds& piece_ids : cells_[from.cell_id()]) {
        collections_[piece_ids.collection_id].model->remove_value(
            piece_ids.piece_id,
            collections_[piece_ids.collection_id].type->value_to_index(value)
        );
        Gecode::SpaceStatus status = collections_[piece_ids.collection_id].model->status();
        if (status == Gecode::SS_FAILED) {
            std::cout << this->to_string() << std::endl;
            throw std::runtime_error("Failed to remove piece value (remove_piece_value)");
        }
        updated_collection_ids.push_back(piece_ids.collection_id);
    }
    for (int collection_id : updated_collection_ids) {
        collections_[collection_id].rbp.update_probabilities(collections_[collection_id].model->get_domains());
    }
}

void State::remove_piece_values(const Position& from, const std::vector<PieceValue>& values) {
    if (from.has_stack_id()) {
        PieceIds piece_ids = cells_[from.cell_id()][from.stack_id()];
        for (const PieceValue& value : values) {
            collections_[piece_ids.collection_id].model->remove_value(
                piece_ids.piece_id,
                collections_[piece_ids.collection_id].type->value_to_index(value)
            );
        }
        Gecode::SpaceStatus status = collections_[piece_ids.collection_id].model->status();
        if (status == Gecode::SS_FAILED) {
            std::cout << this->to_string() << std::endl;
            std::cout << "Position(" << from.cell_id() << ", " << from.stack_id() << ")\n";
            for (const auto& value : values) {
                std::cout << collections_[piece_ids.collection_id].type->value_to_index(value) << " ";
            }
            std::cout << "\n";
            throw std::runtime_error("Failed to remove piece value (remove_piece_values precise)");
        }
        collections_[piece_ids.collection_id].rbp.update_probabilities(collections_[piece_ids.collection_id].model->get_domains());
        return;
    }
    std::vector<int> updated_collection_ids;
    for (const PieceIds& piece_ids : cells_[from.cell_id()]) {
        for (const PieceValue& value : values) {
            collections_[piece_ids.collection_id].model->remove_value(
                piece_ids.piece_id,
                collections_[piece_ids.collection_id].type->value_to_index(value)
            );
        }
        Gecode::SpaceStatus status = collections_[piece_ids.collection_id].model->status();
        if (status == Gecode::SS_FAILED) {
            std::cout << this->to_string() << std::endl;
            std::cout << "Position(" << from.cell_id() << ")\n";
            for (const auto& value : values) {
                std::cout << collections_[piece_ids.collection_id].type->value_to_index(value) << " ";
            }
            std::cout << "\n";
            throw std::runtime_error("Failed to remove piece value (remove_piece_values)");
        }
        updated_collection_ids.push_back(piece_ids.collection_id);
    }
    for (int collection_id : updated_collection_ids) {
        collections_[collection_id].rbp.update_probabilities(collections_[collection_id].model->get_domains());
    }
}

void State::assign_piece_value(const Position& from, const PieceValue& value) {
    if (from.has_stack_id()) {
        PieceIds piece_ids = cells_[from.cell_id()][from.stack_id()];
        collections_[piece_ids.collection_id].model->assign_value(
            piece_ids.piece_id,
            collections_[piece_ids.collection_id].type->value_to_index(value)
        );
        Gecode::SpaceStatus status = collections_[piece_ids.collection_id].model->status();
        if (status == Gecode::SS_FAILED) {
            throw std::runtime_error("Failed to assign piece value");
        }
        collections_[piece_ids.collection_id].rbp.update_probabilities(collections_[piece_ids.collection_id].model->get_domains());
        return;
    }
    std::vector<int> updated_collection_ids;
    for (const PieceIds& piece_ids : cells_[from.cell_id()]) {
        collections_[piece_ids.collection_id].model->assign_value(
            piece_ids.piece_id,
            collections_[piece_ids.collection_id].type->value_to_index(value)
        );
        Gecode::SpaceStatus status = collections_[piece_ids.collection_id].model->status();
        if (status == Gecode::SS_FAILED) {
            throw std::runtime_error("Failed to assign piece value");
        }
        updated_collection_ids.push_back(piece_ids.collection_id);
    }
    for (int collection_id : updated_collection_ids) {
        collections_[collection_id].rbp.update_probabilities(collections_[collection_id].model->get_domains());
    }
}

bool State::add_observers(const Position& from, const std::vector<PlayerId>& observers) {
    if (from.has_stack_id()) {
        PieceIds piece_id = cells_[from.cell_id()][from.stack_id()];
        std::vector<PlayerId>& last_observers = collections_[piece_id.collection_id].observers[piece_id.piece_id];
        last_observers.insert(last_observers.end(), observers.begin(), observers.end());
        std::sort(last_observers.begin(), last_observers.end());
        last_observers.erase(std::unique(last_observers.begin(), last_observers.end()), last_observers.end());
    } else {
        for (const PieceIds& piece_id : cells_[from.cell_id()]) {
            std::vector<PlayerId>& last_observers = collections_[piece_id.collection_id].observers[piece_id.piece_id];
            last_observers.insert(last_observers.end(), observers.begin(), observers.end());
            std::sort(last_observers.begin(), last_observers.end());
            last_observers.erase(std::unique(last_observers.begin(), last_observers.end()), last_observers.end());
        }
    }
    return is_seen(observers);
}

void State::remove_observers(const Position& from, const std::vector<PlayerId>& observers) {
    if (from.has_stack_id()) {
        PieceIds piece_ids = cells_[from.cell_id()][from.stack_id()];
        std::vector<PlayerId> new_observers;
        std::vector<PlayerId>& old_observers = collections_[piece_ids.collection_id].observers[piece_ids.piece_id];
        std::set_difference(old_observers.begin(), old_observers.end(), observers.begin(), observers.end(), std::back_inserter(new_observers));
        old_observers = new_observers;
        return;
    }
    for (const PieceIds& piece_id : cells_[from.cell_id()]) {
        std::vector<PlayerId> new_observers;
        std::vector<PlayerId>& old_observers = collections_[piece_id.collection_id].observers[piece_id.piece_id];
        std::set_difference(old_observers.begin(), old_observers.end(), observers.begin(), observers.end(), std::back_inserter(new_observers));
        old_observers = new_observers;
    }
}

void State::hide(const Position& from) {
    if (from.has_stack_id()) {
        const PieceIds& piece_ids = cells_[from.cell_id()][from.stack_id()];
        collections_[piece_ids.collection_id].observers[piece_ids.piece_id] = {};
    } else {
        for (const PieceIds& piece_ids : cells_[from.cell_id()]) {
            collections_[piece_ids.collection_id].observers[piece_ids.piece_id] = {};
        }
    }
}

void State::shuffle(const Position& from) {
    if (from.has_stack_id()) {
        throw std::invalid_argument("Cannot shuffle a single piece");
    }

    std::unordered_map<std::shared_ptr<const PieceType>, std::vector<int>> counts;
    for (PieceIds piece_ids : cells_[from.cell_id()]) {
        const CollectionWrapper& collection = collections_[piece_ids.collection_id];
        const std::vector<PlayerId>& observers = collection.observers[piece_ids.piece_id];

        if (is_seen(observers)) {
            continue;
        }

        if (!counts.contains(collection.type)) {
            counts.emplace(collection.type, std::vector<int>(collection.type->size(), 0));
        }

        std::vector<bool> values = collection.model->get_domain(piece_ids.piece_id);
        for (int value = 0; value < values.size(); value++) {
            if (values[value]) {
                counts[collection.type][value]++;
            }
        }
    }

    std::vector<std::unique_ptr<CollectionModel>> new_models;
    new_models.reserve(collections_.size());
    for (const auto& collection : collections_) {
        new_models.emplace_back(dynamic_cast<CollectionModel*>(collection.original_model->clone()));
    }
    const int cells_size = static_cast<int>(cells_.size());
    for (int cell_id = 0; cell_id < cells_size; cell_id++) {
        if (cell_id == from.cell_id()) {
            for (const PieceIds& piece_ids : cells_[cell_id]) {

                const CollectionWrapper& collection = collections_[piece_ids.collection_id];
                const std::vector<PlayerId>& observers = collection.observers[piece_ids.piece_id];

                if (is_seen(observers)) {
                    continue;
                }

                std::unique_ptr<CollectionModel>& new_model = new_models[piece_ids.collection_id];
                const std::vector<int>& cell_counts = counts[collection.type];
                for (int value = 0; value < cell_counts.size(); value++) {
                    if (cell_counts[value] == 0) {
                        new_model->remove_value(piece_ids.piece_id, value);
                    }
                }
            }
        } else {
            for (const PieceIds& piece_ids : cells_[cell_id]) {
                const CollectionWrapper& collection = collections_[piece_ids.collection_id];
                std::unique_ptr<CollectionModel>& new_model = new_models[piece_ids.collection_id];
                std::vector<bool> can_be_values = collection.model->get_domain(piece_ids.piece_id);
                for (int value = 0; value < can_be_values.size(); value++) {
                    if (!can_be_values[value]) {
                        new_model->remove_value(piece_ids.piece_id, value);
                    }
                }
            }
        }
    }

    for (auto& new_model : new_models) {
        Gecode::SpaceStatus status = new_model->status();
        if (status == Gecode::SS_FAILED) {
            throw std::logic_error("Cannot shuffle correctly");
        }
    }

    for (int collection_id = 0; collection_id < new_models.size(); collection_id++) {
        collections_[collection_id].model = std::move(new_models[collection_id]);
    }

    for (auto& collection : collections_) {
        collection.rbp.update_probabilities(collection.model->get_domains());
    }
}

bool State::assignment_possible(const Position& from, const std::vector<PieceValue>& not_values) const {
    // TODO !! 
    if (from.has_stack_id()) { // Ça c'est moche quand même
        const PieceIds& piece_ids = cells_[from.cell_id()][from.stack_id()];
        CollectionWrapper collection_copy = collections_[piece_ids.collection_id];
        for (const PieceValue& value : not_values) {
            collection_copy.model->remove_value(
                piece_ids.piece_id,
                collection_copy.type->value_to_index(value)
            );
        }
        Gecode::SpaceStatus status = collection_copy.model->status();
        if (status == Gecode::SS_FAILED) {
            return false;
        }
        Gecode::DFS<CollectionModel> dfs(collection_copy.model.get());
        if (dfs.next()) {
            return true;
        } else {
            return false;
        }
    } else {
        std::unordered_map<int, CollectionWrapper> collection_copies;
        for (const PieceIds& piece_ids : cells_[from.cell_id()]) {
            if (!collection_copies.contains(piece_ids.collection_id)) {
                collection_copies.emplace(piece_ids.collection_id, collections_[piece_ids.collection_id]);
            }
            CollectionWrapper& collection_copy = collection_copies.at(piece_ids.collection_id);
            for (const PieceValue& value : not_values) {
                collection_copy.model->remove_value(
                    piece_ids.piece_id,
                    collection_copy.type->value_to_index(value)
                );
            }
        }
        for (const auto& [_, collection_copy] : collection_copies) {
            Gecode::SpaceStatus status = collection_copy.model->status();
            if (status == Gecode::SS_FAILED) {
                return false;
            }
            Gecode::DFS<CollectionModel> dfs(collection_copy.model.get());
            if (!dfs.next()) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool State::is_consistent_with(const State& other) const {
    for (int cell_id = 0; cell_id < cells_.size(); cell_id++) {
        if (cells_[cell_id].size() != other.cells_[cell_id].size()) {
            return false;
        }

        for (int stack_id = 0; stack_id < cells_[cell_id].size(); stack_id++) {
            if (cells_[cell_id][stack_id].collection_id != other.cells_[cell_id][stack_id].collection_id) {
                return false;
            }
            PieceIds piece_ids = cells_[cell_id][stack_id];
            std::vector<bool> can_be_value = collections_[piece_ids.collection_id].model->get_domain(piece_ids.piece_id);
            PieceIds other_piece_ids = other.cells_[cell_id][stack_id];
            std::vector<bool> other_can_be_value = other.collections_[other_piece_ids.collection_id].model->get_domain(other_piece_ids.piece_id);
            for (int value_id = 0; value_id < can_be_value.size(); value_id++) {
                if (!can_be_value[value_id] && other_can_be_value[value_id]) {
                    return false;
                }
            }
            if (collections_[piece_ids.collection_id].observers[piece_ids.piece_id] != other.collections_[other_piece_ids.collection_id].observers[other_piece_ids.piece_id]) {
                return false;
            }
        }
    }
    return variables_ == other.variables_ && current_players_ == other.current_players_;
}

std::string State::to_string() const {
    std::string s = "State(" + game_->name() + "): \n";
    s += "    Point of view: " + point_of_view_.to_string() + "\n";
    s += "    Current players: ";
    for (PlayerId player_id : current_players_) {
        s += std::to_string(player_id) + " ";
    }
    s += "\n";
    s += "    Cells: \n";
    for (int i = 0; i < cells_.size(); i++) {
        s += "        Cell " + std::to_string(i) + ": ";
        for (const PieceIds& piece_ids : cells_[i]) {
            s += "(" + std::to_string(piece_ids.collection_id) + ", " + std::to_string(piece_ids.piece_id) + ") ";
        }
        s += "\n";
    }
    s += "    Collections: \n";
    for (int collection_id = 0; collection_id < collections_.size(); collection_id++) {
        s += "        Collection " + std::to_string(collection_id) + ": \n";
        for (int piece_id = 0; piece_id < collections_[collection_id].observers.size(); piece_id++) {
            s += "            Piece " + std::to_string(piece_id) + ": \n";
            std::vector<int> values = collections_[collection_id].model->get_values(piece_id);
            s += "                Domain: ";
            for (int value: values) {
                s += std::to_string(value) + "(" + std::to_string(collections_[collection_id].rbp.get_probability(piece_id, value)) + ") ";
            }
            s += "\n";
            s += "                Observers: ";
            for (PlayerId observer : collections_[collection_id].observers[piece_id]) {
                s += std::to_string(observer) + " ";
            }
            s += "\n";
        }
    }
    s += "    Variables: \n";
    for (const Variable& variable : variables_) {
        s += "       " + variable.to_string() + "\n";
    }
    return s;
}

bool State::is_seen(const std::vector<PlayerId>& observers) const {
    if (observers.empty()) {
        return false;
    }

    if (point_of_view_.type == PointOfView::Type::World) {
        return !observers.empty();
    }
    if (point_of_view_.type == PointOfView::Type::Public) {
        return observers.size() == game_->num_players();
    }
    if (point_of_view_.type == PointOfView::Type::Private) {
        return std::ranges::find(observers, point_of_view_.player_id) != observers.end();
    }
    return false;
}

State::PieceIds State::pop_piece_id_from_cell(const Position& position) {
    if (position.has_stack_id()) {
        PieceIds piece_id = cells_[position.cell_id()][position.stack_id()];
        cells_[position.cell_id()].erase(cells_[position.cell_id()].begin() + position.stack_id());
        return piece_id;
    }
    PieceIds piece_id = cells_[position.cell_id()].back();
    cells_[position.cell_id()].pop_back();
    return piece_id;
}

void State::put_piece_id_in_cell(const Position& position, PieceIds piece_id) {
    if (position.has_stack_id()) {
        cells_[position.cell_id()].insert(cells_[position.cell_id()].begin() + position.stack_id(), piece_id);
    } else {
        cells_[position.cell_id()].push_back(piece_id);
    }
}

StateBuilder::StateBuilder(std::shared_ptr<const Game> game, const PointOfView& point_of_view) {
    state_.game_ = std::move(game);
    state_.point_of_view_ = point_of_view;
    state_.cells_ = std::vector<std::vector<State::PieceIds>>(state_.game_->play_graph().size());
}

StateBuilder& StateBuilder::set_initial_players(const std::vector<PlayerId>& player_ids) {
    state_.set_current_players(player_ids);
    return *this;
}

StateBuilder& StateBuilder::add_piece(std::shared_ptr<const PieceType> type, const PieceValue& value, const std::vector<PlayerId>& observers, const Position& position) {
    pieces_.emplace_back(std::move(type), value, observers, position);
    return *this;
}

StateBuilder& StateBuilder::add_variable(const Variable& variable) {
    state_.set_variable(variable);
    return *this;
}

State StateBuilder::build() {

    std::unordered_map<std::shared_ptr<const PieceType>, std::vector<FixedPiece>> piece_map;
    std::unordered_map<std::shared_ptr<const PieceType>, std::vector<int>> piece_count;
    for (const FixedPiece& piece : pieces_) {
        if (piece_map.find(piece.type) == piece_map.end()) {
            piece_map[piece.type] = std::vector<FixedPiece>();
        }
        piece_map[piece.type].push_back(piece);

        if (piece_count.find(piece.type) == piece_count.end()) {
            piece_count[piece.type] = std::vector<int>(piece.type->size(), 0);
        }
        int piece_value_id = piece.type->value_to_index(piece.value);
        piece_count[piece.type][piece_value_id] += 1;
    }

    // /!\ Doesn't take into account the position of the pieces.

    int type_id = 0;
    for (const auto& [type, pieces] : piece_map) {
        state_.collections_.emplace_back(type, pieces.size(), piece_count[type]);

        for (int piece_id = 0; piece_id < pieces.size(); piece_id++) {
            state_.collections_.back().observers[piece_id] = pieces[piece_id].observers;
            if (state_.is_seen(pieces[piece_id].observers)) {
                state_.collections_.back().model->assign_value(piece_id, type->value_to_index(pieces[piece_id].value));
            }
            state_.cells_[pieces[piece_id].position.cell_id()].push_back({type_id, piece_id});
        }
        state_.collections_.back().original_model->status();
        Gecode::SpaceStatus status = state_.collections_.back().model->status();
        if (status == Gecode::SS_FAILED) {
            throw std::runtime_error("Failed to create collection");
        }
        state_.collections_.back().rbp.update_probabilities(state_.collections_.back().model->get_domains());
        type_id++;
    }

    return state_;
}

bool State::is_determined() const {
    for (const auto& collection : collections_) {
        if (collection.model->status() != Gecode::SS_SOLVED) {
            return false;
        }
    }
    return true;
}

double State::determinize(std::mt19937& generator) {

    if (is_determined()) {
        return 1.0;
    }

    double total_probability = 1.0;

    for (const auto& pieces : cells_) {
        for (const auto& piece_ids : pieces) {
            CollectionWrapper& collection = collections_[piece_ids.collection_id];
            std::vector<int> values = collection.model->get_values(piece_ids.piece_id);
            if (values.size() > 1) {
                std::uniform_int_distribution<std::size_t> dist(0, values.size()-1);
                int value = values[dist(generator)];
                total_probability *= collection.rbp.get_probability(piece_ids.piece_id, value);
                collection.model->assign_value(piece_ids.piece_id, value);
                Gecode::SpaceStatus status = collection.model->status();
                if (status == Gecode::SS_FAILED) {
                    throw std::logic_error("Cannot determinize state.");
                }
                collection.rbp.update_probabilities(collection.model->get_domains());
            }
        }
    }

    for (auto& collection : collections_) {
        collection.rbp.update_probabilities(collection.model->get_domains());
    }

    return total_probability;
}

double State::determinize_with_marginals(std::mt19937& generator) {
    double total_probability = 1.0;
    while (!is_determined()) {
        PieceIds max_piece_ids(0, 0);
        double max_prob = -1.0;
        for (const auto& pieces : cells_) {
            for (const auto& piece_ids : pieces) {
                CollectionWrapper& collection = collections_[piece_ids.collection_id];
                std::vector<int> values = collection.model->get_values(piece_ids.piece_id);
                if (values.size() <= 1) {
                    continue;
                }
                double current_max_prob = -1.0;
                for (int value : values) {
                    current_max_prob = std::max(current_max_prob, collection.rbp.get_probability(piece_ids.piece_id, value));
                }
                if (current_max_prob > max_prob) {
                    max_prob = current_max_prob;
                    max_piece_ids = piece_ids;
                }
            }
        }

        if (max_prob < 0.0) {
            throw std::logic_error("Cannot find a piece to determinized while the state is not determined");
        }

        CollectionWrapper& collection = collections_[max_piece_ids.collection_id];
        std::vector<int> values = collection.model->get_values(max_piece_ids.piece_id);
        std::vector<double> probs;
        probs.reserve(values.size());
        for (int value : values) {
            probs.push_back(collection.rbp.get_probability(max_piece_ids.piece_id, value));
        }
        std::discrete_distribution<int> dist(probs.begin(), probs.end());
        int value = values[dist(generator)];

        total_probability *= collection.rbp.get_probability(max_piece_ids.piece_id, value);

        collection.model->assign_value(max_piece_ids.piece_id, value);
        Gecode::SpaceStatus status = collection.model->status();
        if (status == Gecode::SS_FAILED) {
            throw std::logic_error("Cannot determinize state.");
        }
        collection.rbp.update_probabilities(collection.model->get_domains());

    }
    return total_probability;
}

}  // namespace belief_sg
