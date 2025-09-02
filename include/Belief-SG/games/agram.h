#ifndef BELIEF_SG_GAMES_AGRAM_H
#define BELIEF_SG_GAMES_AGRAM_H

#include <memory>
#include <string>
#include <vector>

#include "Belief-SG/core/game.h"
#include "Belief-SG/core/piece_type.h"
#include "Belief-SG/core/play_graph.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/point_of_view.h"

namespace belief_sg {

class Agram : public Game {
public:
    explicit Agram(int num_players);

    [[nodiscard]] std::string name() const override;

    [[nodiscard]] int num_players() const override;
    [[nodiscard]] const PlayGraph& play_graph() const override;

    [[nodiscard]] State initial_state(const PointOfView& point_of_view) const override;

    [[nodiscard]] std::vector<ProbAction> legal_actions(const State& state, PlayerId player_id) const override;

    [[nodiscard]] bool is_terminal(const State& state) const override;
    [[nodiscard]] std::vector<double> returns(const State& state) const override;
private:
    [[nodiscard]] std::vector<PlayerId> all_players() const;

    int num_players_;
    PlayGraph play_graph_;

    std::shared_ptr<const PieceType> card_type_;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_GAMES_AGRAM_H
