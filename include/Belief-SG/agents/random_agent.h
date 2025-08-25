#ifndef BELIEF_SG_AGENTS_RANDOM_AGENT_H
#define BELIEF_SG_AGENTS_RANDOM_AGENT_H

#include <memory>
#include <random>

#include "Belief-SG/core/agent.h"
#include "Belief-SG/core/game.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/action.h"

namespace belief_sg {

class RandomAgent : public Agent {
public:
    RandomAgent();

    void set_game(std::shared_ptr<Game> game) override;
    void set_player(PlayerId player) override;
    Action act(const State& private_state, const State& public_state) override;

private:
    std::mt19937 generator_;
    std::shared_ptr<Game> game_;
    PlayerId player_{0};
};

}  // namespace belief_sg

#endif  //BELIEF_SG_AGENTS_RANDOM_AGENT_H
