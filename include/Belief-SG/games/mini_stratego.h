#ifndef BELIEF_SG_GAMES_MINI_STRATEGO_H
#define BELIEF_SG_GAMES_MINI_STRATEGO_H

#include "Belief-SG/core/game.h"
#include "Belief-SG/core/piece_type.h"
#include "Belief-SG/core/play_graph.h"
#include "Belief-SG/core/point_of_view.h"
#include <memory>

namespace belief_sg {

class BattleStratego : public Move {
public:
    BattleStratego() = default;
    explicit BattleStratego(const Position& from);
    ~BattleStratego() override = default;

    [[nodiscard]] std::vector<ProbTransition> apply(const State& state) const override;
    void apply_inplace(State& state, std::mt19937& generator) const override;
    [[nodiscard]] std::unique_ptr<Move> clone() const override;
private:
    Position from_;

    [[nodiscard]] bool is_equals(const Move& other) const override;
};

class MiniStratego : public Game {
public:
    MiniStratego();

    [[nodiscard]] std::string name() const override;

    [[nodiscard]] int num_players() const override;
    [[nodiscard]] const PlayGraph& play_graph() const override;

    [[nodiscard]] State initial_state(const PointOfView& point_of_view) const override;

    [[nodiscard]] std::vector<ProbAction> legal_actions(const State& state, PlayerId player_id) const override;

    [[nodiscard]] bool is_terminal(const State& state) const override;
    [[nodiscard]] std::vector<double> returns(const State& state) const override;
private:

    int num_players_{2};
    PlayGraph play_graph_;

    std::shared_ptr<const PieceType> blue_stratego_type_;
    std::shared_ptr<const PieceType> red_stratego_type_;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_GAMES_MINI_STRATEGO_H
