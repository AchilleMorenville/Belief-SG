#ifndef STRATEGO_H_
#define STRATEGO_H_

#include "game.h"

enum StrategoPiece {
  FLAG,
  BOMB,
  MINER,
  SOLDIER
};

class StrategoGame : public Game {
public:
  
  StrategoGame();
  ~StrategoGame() override;
  
  BeliefState InitialState(std::vector<int> observers) const override;

  std::vector<StochasticAction> LegalActions(const BeliefState& state, int player) const override;
  std::vector<StochasticTransition> ApplyAction(const BeliefState& state, Action action) const override;

  bool IsTerminal(const BeliefState& state) const override;
  std::vector<double> Returns(const BeliefState& state) const override;

  void print(const BeliefState& state) const override; 

private:

  void CreateDefaultTiles();

  bool Dies(PieceType attacker, PieceType defender) const;

  int64_t EncodeAction(int pos_0, int pos_1) const;
  std::pair<int, int> DecodeAction(int64_t action) const;

  std::vector<PieceType> piece_types_;
  std::unordered_map<int, Tile> default_tiles_;

  std::vector<int> board_;
  std::array<int, 2> hands_;
};

#endif  // STRATEGO_H_