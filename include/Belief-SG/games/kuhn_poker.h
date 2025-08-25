#ifndef BELIEF_SG_GAMES_KUHN_POKER_H
#define BELIEF_SG_GAMES_KUHN_POKER_H

#include "Belief-SG/core/game.h"
#include "Belief-SG/core/piece_type.h"
#include "Belief-SG/core/piece_value.h"
#include "Belief-SG/core/play_graph.h"
#include "Belief-SG/core/point_of_view.h"
#include <memory>

namespace belief_sg {

class KuhnPoker : public Game {
public:
    KuhnPoker();

    [[nodiscard]] std::string name() const override;

    [[nodiscard]] int num_players() const override;
    [[nodiscard]] const PlayGraph& play_graph() const override;

    [[nodiscard]] State initial_state(const PointOfView& point_of_view) const override;

    [[nodiscard]] std::vector<ProbAction> legal_actions(const State& state, PlayerId player_id) const override;

    [[nodiscard]] bool is_terminal(const State& state) const override;
    [[nodiscard]] std::vector<double> returns(const State& state) const override;
private:

    [[nodiscard]] static bool wins(const PieceValue& value_first, const PieceValue& value_second);

    int num_players_{2};
    PlayGraph play_graph_;

    std::shared_ptr<const PieceType> card_type_;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_GAMES_KUHN_POKER_H
