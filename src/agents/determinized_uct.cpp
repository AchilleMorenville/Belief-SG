#include "Belief-SG/agents/determinized_uct.h"

#include "Belief-SG/core/action.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/state.h"

#include <memory>
#include <random>
#include <optional>
#include <unordered_map>
#include <vector>

namespace belief_sg {

NodeUCT::NodeUCT(const std::shared_ptr<Game>& game, const State& state, NodeUCT* parent_node, std::optional<std::vector<Action>> parent_joint_action) : state(state), parent_node(parent_node), parent_joint_action(std::move(parent_joint_action)), n_visits(-1) {
    if (game->is_terminal(state)) {
        return;
    }

    const std::vector<PlayerId>& current_players = state.current_players();
    actions = std::vector<std::vector<NodeUCT::ActionInfo>>(
        current_players.size(),
        std::vector<NodeUCT::ActionInfo>()
    );
    for (int i = 0; i < current_players.size(); ++i) {
        std::vector<ProbAction> legal_actions = game->legal_actions(state, current_players[i]);
        actions[i].reserve(legal_actions.size());
        for (const auto& prob_action : legal_actions) {
            actions[i].push_back(
                {.action = prob_action.action, .n_visits = 0, .sum_results = 0.0}
            );
        }
    }
}

bool NodeUCT::is_fully_expanded() const {
    for (const auto& action_infos : actions) {
        for (const auto& action_info : action_infos) {
            if (action_info.n_visits <= 0) {
                return false;
            }
        }
    }
    return true;
}

DeterminizedUCT::DeterminizedUCT() : generator_(std::random_device{}()), n_samples_(10), n_iterations_(1000), use_prob_(false) {}

DeterminizedUCT::DeterminizedUCT(int n_samples, int n_iterations, bool use_prob) : generator_(std::random_device{}()), n_samples_(n_samples), n_iterations_(n_iterations), use_prob_(use_prob) {}

void DeterminizedUCT::set_game(std::shared_ptr<Game> game) {
    game_ = game;
}

void DeterminizedUCT::set_player(PlayerId player) {
    player_ = player;
}

Action DeterminizedUCT::act(const State& private_state, const State& public_state) {

    std::vector<ProbAction> actions = game_->legal_actions(private_state, player_);
    if (actions.size() == 1) {
        return actions[0].action;
    }

    roots_.clear();
    roots_.reserve(n_samples_);
    for (int sample_i = 0; sample_i < n_samples_; ++sample_i) {
        State determinized_state(private_state);
        if (use_prob_) {
            determinized_state.determinize_with_marginals(generator_);
        } else {
            determinized_state.determinize(generator_);
        }
        roots_.push_back(std::make_unique<NodeUCT>(game_, determinized_state));
        roots_.back()->n_visits++;
    }

    for (const auto& root : roots_) {
        for (int playout_i = 0; playout_i < n_iterations_; ++playout_i) {
            run_playout(root.get());
        }
        root->successors.clear();
    }

    std::vector<std::pair<Action, int>> action_visits;
    for (int sample_id = 0; sample_id < n_samples_; ++sample_id) {
        std::vector<PlayerId> current_players = roots_[sample_id]->state.current_players();
        auto it = std::ranges::find(current_players, player_);
        if (it == current_players.end()) {
            throw std::runtime_error("Player not found in current players");
        }
        std::size_t action_id = std::distance(current_players.begin(), it);
        for (const auto& action_info : roots_[sample_id]->actions[action_id]) {
            auto it = std::ranges::find_if(action_visits, [&](const auto& pair) {
                return pair.first == action_info.action;
            });
            if (it == action_visits.end()) {
                action_visits.emplace_back(action_info.action, action_info.n_visits);
            } else {
                it->second += action_info.n_visits;
            }
        }
    }

    Action max_action = action_visits[0].first;
    int max_visits = action_visits[0].second;
    for (const auto& pair : action_visits) {
        if (pair.second > max_visits) {
            max_action = pair.first;
            max_visits = pair.second;
        }
    }

    roots_.clear();

    return max_action;
}

void DeterminizedUCT::run_playout(NodeUCT* root) {
    NodeUCT* child = select_and_expand(root);
    std::vector<double> result = simulate(child);
    backpropagate(child, result);
}

std::vector<Action> DeterminizedUCT::select_joint_action(NodeUCT* node) {
    const std::vector<PlayerId>& current_players = node->state.current_players();
    if (current_players.size() == 1 && current_players[0] == kChancePlayerId) {
        double min_n_visits = std::numeric_limits<double>::infinity();
        Action action;
        for (const auto& action_info : node->actions[0]) {
            if (action_info.n_visits < min_n_visits) {
                min_n_visits = action_info.n_visits;
                action = action_info.action;
            }
        }
        return {action};
    }
    std::vector<Action> joint_action;
    joint_action.reserve(node->actions.size());
    for (int i = 0; i < node->actions.size(); ++i) {
        const auto& action_infos = node->actions[i];
        int total_visits = node->n_visits;
        double log_total = std::log(std::max(1, total_visits));
        double best_score = -std::numeric_limits<double>::infinity();
        int best_idx = -1;
        for (int j = 0; j < action_infos.size(); ++j) {
            if (action_infos[j].n_visits == 0) {
                best_idx = j;
                break;
            }
            double ucb1_score = (action_infos[j].sum_results / action_infos[j].n_visits) + sqrt(2 * log_total / action_infos[j].n_visits);
            if (ucb1_score > best_score) {
                best_score = ucb1_score;
                best_idx = j;
            }
        }
        joint_action.push_back(action_infos[best_idx].action);
    }
    return joint_action;
}

NodeUCT* DeterminizedUCT::select_and_expand(NodeUCT* node) {
    while (!game_->is_terminal(node->state)) {
        std::vector<Action> joint_action = select_joint_action(node);
        auto it = std::find_if(
            node->successors.begin(), node->successors.end(),
            [&](const NodeUCT::SuccessorInfo& s) { return s.joint_action == joint_action; }
        );
        if (it == node->successors.end()) {
            // No successor yet — create the leaf and return it;
            State new_state(node->state);
            game_->apply_joint_action_inplace(new_state, joint_action, generator_);
            node->successors.push_back({
                .joint_action = joint_action,
                .successor = std::make_unique<NodeUCT>(game_, new_state, node, joint_action)
            });
            node = node->successors.back().successor.get();
            break;
        }
        node = it->successor.get();
    }
    return node;
}

std::vector<double> DeterminizedUCT::simulate(NodeUCT* node) {
    State current_state(node->state);
    int playout_iter = 0;
    while (!game_->is_terminal(current_state) && playout_iter < 200) {
        const auto& current_players = current_state.current_players();
        std::vector<Action> joint_action(current_players.size());
        for (PlayerId player_id : current_players) {
            std::vector<ProbAction> prob_actions = game_->legal_actions(current_state, player_id);
            std::uniform_int_distribution<int> distribution(0, static_cast<int>(prob_actions.size()) - 1);
            joint_action.push_back(prob_actions[distribution(generator_)].action);
        }
        game_->apply_joint_action_inplace(current_state, joint_action, generator_);
        playout_iter++;
    }
    return game_->returns(current_state);
}

void DeterminizedUCT::backpropagate(NodeUCT* node, const std::vector<double>& result) {
    while (node != nullptr) {
        node->n_visits++;
        const auto& joint_action = node->parent_joint_action;
        if (joint_action.has_value() && node->parent_node != nullptr) {
            const std::vector<Action>& ja = joint_action.value();
            auto& parent_actions = node->parent_node->actions;
            const std::vector<PlayerId>& parent_players = node->parent_node->state.current_players();
            for (int i = 0; i < ja.size(); ++i) {
                for (auto& info : parent_actions[i]) {
                    if (info.action == ja[i]) {
                        info.n_visits++;
                        info.sum_results += result[parent_players[i]];
                        break;
                    }
                }
            }
        }
        node = node->parent_node;
    }
}

}  // namespace belief_sg
