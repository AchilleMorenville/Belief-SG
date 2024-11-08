#ifndef AGENT_H_
#define AGENT_H_

#include <memory>

#include "game.h"

class Agent {
public:
  virtual ~Agent() {};
  virtual void SetGame(std::shared_ptr<Game> game) = 0;
  virtual void SetPlayer(int player) = 0;
  virtual Action Act(const BeliefState& state) = 0;
};

#endif  // AGENT_H_