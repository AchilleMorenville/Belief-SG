#include "Belief-SG/core/game.h"
#include "Belief-SG/core/prob_transition.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/action.h"

namespace belief_sg {

std::vector<ProbTransition> Game::apply_joint_action(const State& state, const std::vector<Action>& joint_action) const {
    std::vector<ProbTransition> transitions;
    transitions.push_back(ProbTransition({state, 1.0}));

    for (const auto& action : joint_action) {
        std::vector<ProbTransition> new_transitions;
        for (const auto& transition : transitions) {
            for (const auto& new_transition : action.apply(transition.state)) {
                new_transitions.push_back(ProbTransition({new_transition.state, new_transition.probability * transition.probability}));
            }
        }
        transitions = new_transitions;
    }
    return transitions;
}

void Game::apply_joint_action_inplace(State& state, const std::vector<Action>& joint_action, std::mt19937& generator) const {
    for (const auto& action : joint_action) {
        action.apply_inplace(state, generator);
    }
}

}  // namespace belief_sg
