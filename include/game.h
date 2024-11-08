#ifndef GAME_H_
#define GAME_H_

#include <vector>
#include <unordered_map>
#include <memory>

struct PieceType {
  std::vector<int> values;

  PieceType(std::vector<int> values);
  bool operator==(const PieceType& other) const;
};

template<> struct std::hash<PieceType> {
  std::size_t operator()(const PieceType& type) const {
    std::size_t seed = 0;
    for (const auto& value : type.values) {
      seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

class Piece {
public:
  Piece() = default;
  Piece(int id, int owner, const std::vector<PieceType>& types, int tile_id);
  Piece(int id, int owner, const std::vector<PieceType>& types, PieceType type, int tile_id);
  ~Piece();

  std::unordered_map<PieceType, double> GetProbabilities() const;
  double GetProbability(PieceType type) const;
  void SetProbabilities(const std::unordered_map<PieceType, double>& probabilities);

  int GetTileId() const;
  void SetTileId(int tile_id);

  int GetOwner() const;

  int GetId() const;

private:
  int id_;
  std::unordered_map<PieceType, double> probabilities_;
  int owner_;
  int tile_id_;
};

class Tile {
public:
  Tile() = default;
  Tile(int id);
  ~Tile();

  std::vector<int> GetPieceIds() const;
  int GetPieceId(int idx) const;
  void AddPieceId(int piece_id);
  void InsertPieceId(int piece_id, int idx);
  void RemovePieceIdFromIdx(int idx);
  void RemovePieceId(int piece_id);
  
  std::vector<int> GetNeighborIds() const;
  void AddNeighborId(int tile_id);

  int GetId() const;

  bool IsEmpty() const;

private:
  int id_;
  std::vector<int> piece_ids_;
  std::vector<int> neighbor_ids_;
};

class BeliefState {
public:
  BeliefState() = default;
  BeliefState(std::vector<int> observers);
  ~BeliefState();

  void InitializeTiles(const std::unordered_map<int, Tile>& tiles);
  
  void AddPiece(int owner, const std::vector<PieceType>& types, PieceType type, int tile_id);
  std::unordered_map<int, Piece> GetPieces() const;
  Piece GetPiece(int piece_id) const;

  std::unordered_map<int, Tile> GetTiles() const;
  Tile GetTile(int tile_id) const;
  
  int GetCurrentPlayer() const;
  void SetCurrentPlayer(int player);

  void MovePiece(int tile_0_id, int tile_0_idx, int tile_1_id, int tile_1_idx);
  void RemovePiece(int piece_id);

  void Hide(const std::vector<int>& tile_ids, const std::vector<int>& observers);
  void UpdateBelief(int piece_id, PieceType piece_type, double p);

  void print() const;

private:
  std::vector<int> observers_;
  int current_player_;
  int count_pieces_ = 0;
  std::unordered_map<int, Piece> pieces_;

  int count_tiles_ = 0;
  std::unordered_map<int, Tile> tiles_;
};

using Action = int64_t;

struct StochasticAction {
  Action action;
  double p;

  StochasticAction(Action action, double p);
};

struct StochasticTransition {
  BeliefState next_state;
  double p;

  StochasticTransition(BeliefState next_state, double p);
};

class Game {
public:
  virtual ~Game() {};
  virtual BeliefState InitialState(std::vector<int> observers) const = 0;
  virtual std::vector<StochasticAction> LegalActions(const BeliefState& state, int player) const = 0;
  virtual std::vector<StochasticTransition> ApplyAction(const BeliefState& state, Action action) const = 0;
  virtual bool IsTerminal(const BeliefState& state) const = 0;
  virtual std::vector<double> Returns(const BeliefState& state) const = 0;
  virtual void print(const BeliefState& state) const = 0;
};

#endif  // GAME_H_