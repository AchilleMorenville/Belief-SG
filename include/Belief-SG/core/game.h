#ifndef BELIEF_SG_CORE_GAME_H
#define BELIEF_SG_CORE_GAME_H

#include <memory>
#include <vector>
#include <random>

#include "Belief-SG/core/play_graph.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/point_of_view.h"
#include "Belief-SG/core/prob_transition.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/action.h"

namespace belief_sg {

class Game : public std::enable_shared_from_this<Game> {
public:
    virtual ~Game() = default;

    [[nodiscard]] virtual std::string name() const = 0;

    [[nodiscard]] virtual int num_players() const = 0;
    [[nodiscard]] virtual const PlayGraph& play_graph() const = 0;

    [[nodiscard]] virtual State initial_state(const PointOfView& point_of_view) const = 0;

    [[nodiscard]] virtual std::vector<ProbAction> legal_actions(const State& state, PlayerId player_id) const = 0;

    [[nodiscard]] virtual std::vector<ProbTransition> apply_joint_action(const State& state, const std::vector<Action>& joint_action) const;
    virtual void apply_joint_action_inplace(State& state, const std::vector<Action>& joint_action, std::mt19937& generator) const;

    [[nodiscard]] virtual bool is_terminal(const State& state) const = 0;
    [[nodiscard]] virtual std::vector<double> returns(const State& state) const = 0;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_GAME_H
