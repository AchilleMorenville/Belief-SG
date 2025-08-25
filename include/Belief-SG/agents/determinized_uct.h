#ifndef BELIEF_SG_AGENTS_DETERMINIZED_UCT_H
#define BELIEF_SG_AGENTS_DETERMINIZED_UCT_H

#include <memory>
#include <optional>
#include <random>
#include <vector>

#include "Belief-SG/core/agent.h"
#include "Belief-SG/core/game.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/action.h"

namespace belief_sg {

struct NodeUCT {

    struct SuccessorInfo {
        std::vector<Action> joint_action;
        std::unique_ptr<NodeUCT> successor;
    };

    struct ActionInfo {
        Action action;
        int n_visits;
        double sum_results;
    };

    State state;

    NodeUCT* parent_node;
    std::optional<std::vector<Action>> parent_joint_action;

    int n_visits;
    std::vector<std::vector<ActionInfo>> actions;
    std::vector<SuccessorInfo> successors;

    NodeUCT(const std::shared_ptr<Game>& game, const State& state, NodeUCT* parent_node = nullptr, std::optional<std::vector<Action>> parent_joint_action = std::nullopt);

    bool is_fully_expanded() const;
};

class DeterminizedUCT : public Agent {
public:
    DeterminizedUCT();
    DeterminizedUCT(int n_samples, int n_iterations, bool use_prob);

    void set_game(std::shared_ptr<Game> game) override;
    void set_player(PlayerId player) override;

    Action act(const State& private_state, const State& public_state) override;
private:
    void run_playout(NodeUCT* root);

    std::vector<Action> select_joint_action(NodeUCT* node);

    NodeUCT* select_and_expand(NodeUCT* node);
    std::vector<double> simulate(NodeUCT* node);
    void backpropagate(NodeUCT* node, const std::vector<double>& result);

    std::mt19937 generator_;
    int n_samples_;
    int n_iterations_;
    bool use_prob_;

    std::shared_ptr<Game> game_;
    PlayerId player_{0};

    std::vector<std::unique_ptr<NodeUCT>> roots_;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_AGENTS_DETERMINIZED_UCT_H
