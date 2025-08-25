#include "Belief-SG/agents/determinized_mc.h"

#include <cstddef>
#include <memory>
#include <vector>
#include <random>
#include <iostream>

#include "Belief-SG/core/action.h"
#include "Belief-SG/core/game.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/state.h"

namespace belief_sg {

DeterminizedMC::DeterminizedMC() : generator_(std::random_device{}()), n_samples_(10), n_iterations_(1000), use_prob_(false) {}

DeterminizedMC::DeterminizedMC(int n_samples, int n_iterations, bool use_prob) : generator_(std::random_device{}()), n_samples_(n_samples), n_iterations_(n_iterations), use_prob_(use_prob) {}

void DeterminizedMC::set_game(std::shared_ptr<Game> game) {
    game_ = game;
}

void DeterminizedMC::set_player(PlayerId player) {
    player_ = player;
}

Action DeterminizedMC::act(const State& private_state, const State& public_state) {
    std::vector<ProbAction> actions = game_->legal_actions(private_state, player_);

    if (actions.size() == 1) {
        return actions.back().action;
    }

    std::vector<ActionInfo> action_infos(actions.size());
    for (size_t i = 0; i < actions.size(); ++i) {
        action_infos[i].action = actions[i].action;
        action_infos[i].total_reward = 0.0;
        action_infos[i].visit_count = 0;
    }

    std::vector<State> determinized_states;
    determinized_states.reserve(n_samples_);
    for (int i = 0; i < n_samples_; i++) {
        State state(private_state);
        if (use_prob_) {
            state.determinize_with_marginals(generator_);
        } else {
            state.determinize(generator_);
        }
        determinized_states.push_back(state);
    }

    int iter = 0;

    double total_time = 0.0;

    for (ActionInfo& action_info : action_infos) {
        for (const State& determinized_state : determinized_states) {
            for (int i = 0; i < n_iterations_ / n_samples_; ++i) {

                auto start_time = std::chrono::high_resolution_clock::now();
                State state(determinized_state);

                std::vector<Action> joint_action(state.current_players().size());
                for (PlayerId player_id : state.current_players()) {
                    if (player_id == player_) {
                        joint_action.push_back(action_info.action);
                    } else {
                        std::vector<ProbAction> prob_actions = game_->legal_actions(state, player_id);
                        std::uniform_int_distribution<int> distribution(0, static_cast<int>(prob_actions.size()) - 1);
                        joint_action.push_back(prob_actions[distribution(generator_)].action);
                    }
                }

                game_->apply_joint_action_inplace(state, joint_action, generator_);

                int playout_iter = 0;
                while (!game_->is_terminal(state) && playout_iter < 200) {
                    std::vector<Action> joint_action(state.current_players().size());
                    for (PlayerId player_id : state.current_players()) {
                        std::vector<ProbAction> prob_actions = game_->legal_actions(state, player_id);
                        std::uniform_int_distribution<int> distribution(0, static_cast<int>(prob_actions.size()) - 1);
                        joint_action.push_back(prob_actions[distribution(generator_)].action);
                    }

                    game_->apply_joint_action_inplace(state, joint_action, generator_);

                    playout_iter++;
                }


                action_info.total_reward += game_->returns(state)[player_];
                action_info.visit_count++;

                iter++;

                auto end_time = std::chrono::high_resolution_clock::now();

                total_time += std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();

                if (iter % 1000 == 0) {
                    std::cout << total_time / iter << "\n";
                }
            }
        }
    }

    std::cout << total_time << "\n";

    Action max_action = action_infos[0].action;
    double max_expected_reward = action_infos[0].total_reward / action_infos[0].visit_count;

    for (size_t i = 1; i < action_infos.size(); ++i) {
        if (action_infos[i].total_reward / action_infos[i].visit_count > max_expected_reward) {
            max_action = action_infos[i].action;
            max_expected_reward = action_infos[i].total_reward / action_infos[i].visit_count;
        }
    }

    return max_action;
}

}  // namespace belief_sg
