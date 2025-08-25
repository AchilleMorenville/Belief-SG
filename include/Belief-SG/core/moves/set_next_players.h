#ifndef BELIEF_SG_CORE_MOVES_SET_NEXT_PLAYERS_H
#define BELIEF_SG_CORE_MOVES_SET_NEXT_PLAYERS_H

#include <memory>
#include <vector>

#include "Belief-SG/core/move.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/prob_transition.h"

namespace belief_sg {

class SetNextPlayers : public Move {
public:
    SetNextPlayers() = default;
    explicit SetNextPlayers(const std::vector<PlayerId>& next_player_ids);
    ~SetNextPlayers() override = default;

    [[nodiscard]] std::vector<ProbTransition> apply(const State& state) const override;
    void apply_inplace(State& state, std::mt19937& generator) const override;

    [[nodiscard]] std::unique_ptr<Move> clone() const override;
private:
    std::vector<PlayerId> next_player_ids_;

    [[nodiscard]] bool is_equals(const Move& other) const override;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_MOVES_SET_NEXT_PLAYERS_H
