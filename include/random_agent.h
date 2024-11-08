#ifndef RANDOM_AGENT_H_
#define RANDOM_AGENT_H_

#include <random>
#include <memory>

#include <agent.h>
#include <game.h>

class RandomAgent : public Agent {
public:
  RandomAgent();
  ~RandomAgent() override;

  void SetGame(std::shared_ptr<Game> game) override;
  void SetPlayer(int player) override;
  Action Act(const BeliefState& state) override;

private:
  std::mt19937 generator_;
  std::shared_ptr<Game> game_;
  int player_;
};

#endif  // RANDOM_AGENT_H_