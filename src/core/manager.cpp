#include "Belief-SG/core/manager.h"

#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>
#include <random>

#include "Belief-SG/core/action.h"
#include "Belief-SG/core/game.h"
#include "Belief-SG/core/agent.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/point_of_view.h"
#include "Belief-SG/core/prob_transition.h"

namespace belief_sg {

Manager::Manager(std::shared_ptr<Game> game, std::vector<std::unique_ptr<Agent>> agents) : game_(std::move(game)), agents_(std::move(agents)) {
    world_state_ = game_->initial_state(PointOfView(PointOfView::Type::World));
    for (PlayerId player_id = 0; player_id < game_->num_players(); player_id++) {
        private_states_.push_back(game_->initial_state(PointOfView(PointOfView::Type::Private, player_id)));
    }
    public_state_ = game_->initial_state(PointOfView(PointOfView::Type::Public));

    for (PlayerId player_id = 0; player_id < game_->num_players(); player_id++) {
        agents_[player_id]->set_game(game_);
        agents_[player_id]->set_player(player_id);
    }

    generator_ = std::mt19937(std::random_device{}());
}

std::vector<double> Manager::play(bool verbose) {

    int step = 0;

    while (!game_->is_terminal(world_state_)) {
        if (verbose) {
            std::cout << "Step " << step << "\n";
            std::cout << world_state_.to_string() << "\n";
        }

        std::vector<Action> actions;
        for (const PlayerId& player_id : world_state_.current_players()) {

            std::vector<ProbAction> legal_actions = game_->legal_actions(world_state_, player_id);

            if (verbose) {
                std::cout << "Actions for current player : " << player_id << "\n";
                std::cout << "Legal actions size : " << legal_actions.size() << "\n";
            }

            if (legal_actions.empty()) {
                if (verbose) {
                    std::cout << "Player " << player_id << " has no legal actions.\n";
                }
                break;
            }

            Action action;
            if (player_id == kChancePlayerId) {
                std::uniform_int_distribution<int> distribution(0, legal_actions.size() - 1);
                action = legal_actions[distribution(generator_)].action;
            } else {
                action = agents_[player_id]->act(private_states_[player_id], public_state_);
            }
            if (std::ranges::find(legal_actions, action, &ProbAction::action) == legal_actions.end()) {
                if (verbose) {
                    std::cout << "Illegal action\n";
                    std::cout << private_states_[player_id].to_string() << "\n";
                }
                break;
            }

            actions.push_back(action);
        }

        if (actions.size() != world_state_.current_players().size()) {
            if (verbose) {
                std::cout << "Not all players have chosen an action\n";
            }
            break;
        }

        std::vector<ProbTransition> world_transitions = game_->apply_joint_action(world_state_, actions);
        std::uniform_int_distribution<int> distribution(0, world_transitions.size() - 1);
        world_state_ = world_transitions[distribution(generator_)].state;

        for (PlayerId player_id = 0; player_id < game_->num_players(); player_id++) {

            std::vector<ProbTransition> private_transitions = game_->apply_joint_action(private_states_[player_id], actions);
            bool found_consistent = false;
            for (const ProbTransition& private_transition : private_transitions) {

                if (private_transition.state.is_consistent_with(world_state_)) {
                    private_states_[player_id] = private_transition.state;
                    found_consistent = true;
                    break;
                }
            }

            if (!found_consistent) {
                if (verbose) {
                    std::cout << "Player " << player_id << " private state is inconsistent\n";
                }
                break;
            }
        }

        bool public_found_consistent = false;
        std::vector<ProbTransition> public_transitions = game_->apply_joint_action(public_state_, actions);
        for (const ProbTransition& public_transition : public_transitions) {

            if (public_transition.state.is_consistent_with(world_state_)) {
                public_state_ = public_transition.state;
                public_found_consistent = true;
                break;
            }
        }

        if (!public_found_consistent) {
            if (verbose) {
                std::cout << "Public state is inconsistent\n";
            }
            break;
        }

        step++;
    }

    if (verbose) {
        std::cout << "Step " << step << "\n";
        std::cout << world_state_.to_string() << "\n";
        std::cout << "Game is terminal\n";

        for (PlayerId player_id = 0; player_id < game_->num_players(); player_id++) {
            std::cout << "Player " << player_id << " private state\n";
            std::cout << private_states_[player_id].to_string() << "\n";
            if (!game_->is_terminal(private_states_[player_id])) {
                std::cout << "Player " << player_id << " private state is not terminal\n";
            }
        }

        std::cout << "Public state\n";
        std::cout << public_state_.to_string() << "\n";
        if (!game_->is_terminal(public_state_)) {
            std::cout << "Public state is not terminal\n";
        }

        std::vector<double> returns = game_->returns(world_state_);
        for (PlayerId player_id = 0; player_id < game_->num_players(); player_id++) {
            std::cout << "Player " << player_id << " returns : " << returns[player_id] << "\n";
        }
    }

    return game_->returns(world_state_);
}

}  // namespace belief_sg
