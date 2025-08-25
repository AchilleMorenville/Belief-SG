#ifndef BELIEF_SG_CORE_MANAGER_H
#define BELIEF_SG_CORE_MANAGER_H

#include <memory>
#include <random>
#include <vector>

#include "Belief-SG/core/game.h"
#include "Belief-SG/core/agent.h"

namespace belief_sg {

class Manager {
public:
    Manager(std::shared_ptr<Game> game, std::vector<std::unique_ptr<Agent>> agents);

    std::vector<double> play(bool verbose = false);

private:
    std::shared_ptr<Game> game_;
    std::vector<std::unique_ptr<Agent>> agents_;

    State world_state_;
    std::vector<State> private_states_;
    State public_state_;

    std::mt19937 generator_;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_MANAGER_H
