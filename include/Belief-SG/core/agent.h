#ifndef BELIEF_SG_CORE_AGENT_H
#define BELIEF_SG_CORE_AGENT_H

#include <memory>

#include "Belief-SG/core/game.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/action.h"

namespace belief_sg {

class Agent {
public:
    virtual ~Agent() = default;
    virtual void set_game(std::shared_ptr<Game> game) = 0;
    virtual void set_player(PlayerId player) = 0;
    virtual Action act(const State& private_state, const State& public_state) = 0;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_AGENT_H
