#ifndef BELIEF_SG_AGENTS_DETERMINIZED_MC_H
#define BELIEF_SG_AGENTS_DETERMINIZED_MC_H

#include <memory>
#include <random>

#include "Belief-SG/core/agent.h"
#include "Belief-SG/core/game.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/action.h"

namespace belief_sg {

class DeterminizedMC : public Agent {
public:
    DeterminizedMC();
    explicit DeterminizedMC(int n_samples, int n_iterations, bool use_prob);
    void set_game(std::shared_ptr<Game> game) override;
    void set_player(PlayerId player) override;
    Action act(const State& private_state, const State& public_state) override;
private:

    struct ActionInfo {
        Action action;
        double total_reward;
        int visit_count;
    };

    std::shared_ptr<Game> game_;
    PlayerId player_{0};

    std::mt19937 generator_;
    int n_samples_;
    int n_iterations_;
    bool use_prob_;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_AGENTS_DETERMINIZED_MC_H
