#ifndef BELIEF_SG_CORE_STATE_H
#define BELIEF_SG_CORE_STATE_H

#include <memory>
#include <vector>
#include <string>
#include <random>

#include <gecode/int.hh>
#include <gecode/search.hh>

#include "Belief-SG/core/piece_attribute.h"
#include "Belief-SG/core/piece_type.h"
#include "Belief-SG/core/piece_value.h"
#include "Belief-SG/core/point_of_view.h"
#include "Belief-SG/core/position.h"
#include "Belief-SG/core/variable.h"
#include "Belief-SG/core/player_id.h"

namespace belief_sg {

struct Piece {
    std::shared_ptr<const PieceType> type;
    std::vector<PlayerId> observers;
    std::vector<PieceValue> values;
    std::vector<double> probs;

    [[nodiscard]] bool can_be(const PieceValue& value) const;

    [[nodiscard]] bool can_have(const PieceAttribute& attribute) const;
    [[nodiscard]] bool can_not_have(const PieceAttribute& attribute) const;
    
    [[nodiscard]] double probability(const PieceValue& value) const;
};

class CollectionModel : public Gecode::Space {
protected:
    Gecode::IntVarArray pieces_;
    int n_pieces_;
    int n_values_;
public:
    CollectionModel(int n_pieces, int n_values);
    CollectionModel(CollectionModel &s);

    Gecode::Space* copy() override;

    [[nodiscard]] int get_value(int id) const;
    [[nodiscard]] std::vector<int> get_values(int id) const;
    [[nodiscard]] std::vector<bool> get_domain(int id) const;
    [[nodiscard]] std::vector<std::vector<bool>> get_domains() const;

    void remove_value(int id, int value);
    void assign_value(int id, int value);

    void add_counts(std::vector<int> counts);

    void print() const;
};

class BeliefPropagation {
public:
    BeliefPropagation(int n_pieces, int n_values, const std::vector<int>& counts);
    BeliefPropagation() = default;

    void update_probabilities(const std::vector<std::vector<bool>>& domains);

    [[nodiscard]] double get_probability(int id, int value) const {
        return probabilities_[id][value];
    }

private:
    void reset_variables_messages_and_marginals();
    void reset_constraints_messages();

    void compute_constraints_messages();

    void compute_constraint_messages(int constraint_id);
    void normalize_constraint_messages(int constraint_id);

    double compute_variables_messages_and_marginals();

    double compute_variable_messages_and_marginals(int variable_id);
    void normalize_variable_messages(int variable_id);
    void normalize_variable_marginals(int variable_id);

    static const int n_iter_ = 100;
    static constexpr double epsilon_ = 1e-6;
    double damping_ = 0.5;

    int n_variables_;
    int n_values_;
    int n_constraints_;

    std::vector<int> counts_;
    std::vector<int> values_;

    std::vector<std::vector<std::vector<double>>> variable_messages_;
    std::vector<std::vector<double>> variable_marginals_;

    std::vector<std::vector<std::vector<double>>> constraint_messages_;

    std::vector<std::vector<bool>> domains_;
    std::vector<std::vector<double>> probabilities_;
};

struct CollectionWrapper {
    std::shared_ptr<const PieceType> type;
    std::unique_ptr<CollectionModel> original_model;
    std::unique_ptr<CollectionModel> model;
    BeliefPropagation rbp;
    std::vector<std::vector<PlayerId>> observers;

    CollectionWrapper() = default;
    CollectionWrapper(std::shared_ptr<const PieceType> ptype, int n_pieces, const std::vector<int>& counts);
    ~CollectionWrapper() = default;

    CollectionWrapper(const CollectionWrapper& other);
    CollectionWrapper& operator=(const CollectionWrapper& other);

    void swap(CollectionWrapper& other) noexcept;
};

class Game;

class StateBuilder;

class State {
public:
    State() = default;

    [[nodiscard]] std::shared_ptr<const Game> game() const;
    [[nodiscard]] PointOfView point_of_view() const;

    [[nodiscard]] const std::vector<PlayerId>& current_players() const;
    void set_current_player(PlayerId player_id);
    void set_current_players(const std::vector<PlayerId>& player_ids);

    [[nodiscard]] Piece get_piece_at(const Position& position) const;
    [[nodiscard]] std::vector<Piece> get_pieces_at(const Position& position) const;

    [[nodiscard]] Variable variable(const std::string& name) const;
    void set_variable(const Variable& variable);

    void move_piece(const Position& from, const Position& to);
    void remove_piece(const Position& from);

    void remove_piece_value(const Position& from, const PieceValue& value);
    void remove_piece_values(const Position& from, const std::vector<PieceValue>& values);

    void assign_piece_value(const Position& from, const PieceValue& value);

    bool add_observers(const Position& from, const std::vector<PlayerId>& observers);
    void remove_observers(const Position& from, const std::vector<PlayerId>& observers);
    void hide(const Position& from);

    void shuffle(const Position& from);

    [[nodiscard]] bool assignment_possible(const Position& from, const std::vector<PieceValue>& not_values) const;

    [[nodiscard]] bool is_consistent_with(const State& other) const;

    [[nodiscard]] bool is_determined() const;

    double determinize(std::mt19937& generator);
    double determinize_with_marginals(std::mt19937& generator);

    [[nodiscard]] std::string to_string() const;
private:

    [[nodiscard]] bool is_seen(const std::vector<PlayerId>& observers) const;

    struct PieceIds {
        int collection_id;
        int piece_id;
    };

    PieceIds pop_piece_id_from_cell(const Position& position);
    void put_piece_id_in_cell(const Position& position, PieceIds piece_id);

    std::shared_ptr<const Game> game_;
    PointOfView point_of_view_;
    std::vector<PlayerId> current_players_;

    std::vector<std::vector<PieceIds>> cells_;

    std::vector<CollectionWrapper> collections_;

    std::vector<Variable> variables_;

    friend class StateBuilder;
};

class StateBuilder {
public:
    StateBuilder(std::shared_ptr<const Game> game, const PointOfView& point_of_view);

    StateBuilder& set_initial_players(const std::vector<PlayerId>& player_ids);
    StateBuilder& add_piece(std::shared_ptr<const PieceType> type, const PieceValue& value, const std::vector<PlayerId>& observers, const Position& position);
    StateBuilder& add_variable(const Variable& variable);

    [[nodiscard]] State build();
private:
    struct FixedPiece {
        std::shared_ptr<const PieceType> type;
        PieceValue value;
        std::vector<PlayerId> observers;
        Position position;
    };
    std::vector<FixedPiece> pieces_;

    State state_;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_STATE_H
