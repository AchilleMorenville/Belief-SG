#ifndef BELIEF_SG_CORE_PROB_TRANSITION_H
#define BELIEF_SG_CORE_PROB_TRANSITION_H

#include "Belief-SG/core/state.h"

namespace belief_sg {

struct ProbTransition {
    State state;
    double probability{};
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_PROB_TRANSITION_H
