#include "game.h"

#include <iostream>
#include <algorithm>
#include <iomanip>

#include <unordered_set>

PieceType::PieceType(std::vector<int> values) : values(values) {}

bool PieceType::operator==(const PieceType& other) const {
  return values == other.values;
}

Piece::Piece(int id, int owner, const std::vector<PieceType>& types, int tile_id) : id_(id), owner_(owner), tile_id_(tile_id) {
  for (const auto& type : types) {
    probabilities_[type] = 0.0;
  }
}

Piece::Piece(int id, int owner, const std::vector<PieceType>& types, PieceType type, int tile_id) : id_(id), owner_(owner), tile_id_(tile_id) {
  for (const auto& t : types) {
    probabilities_[t] = 0.0;
  }
  probabilities_[type] = 1.0;
}

Piece::~Piece() {}

std::unordered_map<PieceType, double> Piece::GetProbabilities() const {
  return probabilities_;
}

void Piece::SetProbabilities(const std::unordered_map<PieceType, double>& probabilities) {
  probabilities_ = probabilities;
}

int Piece::GetTileId() const {
  return tile_id_;
}

void Piece::SetTileId(int tile_id) {
  tile_id_ = tile_id;
}

int Piece::GetOwner() const {
  return owner_;
}

int Piece::GetId() const {
  return id_;
}

double Piece::GetProbability(PieceType type) const {
  return probabilities_.at(type);
}

Tile::Tile(int id) : id_(id) {}

Tile::~Tile() {}

std::vector<int> Tile::GetPieceIds() const {
  return piece_ids_;
}

int Tile::GetPieceId(int idx) const {
  return piece_ids_[idx];
}

void Tile::AddPieceId(int piece_id) {
  piece_ids_.push_back(piece_id);
}

void Tile::InsertPieceId(int piece_id, int idx) {
  piece_ids_.insert(piece_ids_.begin() + idx, piece_id);
}

void Tile::RemovePieceIdFromIdx(int idx) {
  piece_ids_.erase(piece_ids_.begin() + idx);
}

void Tile::RemovePieceId(int piece_id) {
  for (int i = 0; i < piece_ids_.size(); i++) {
    if (piece_ids_[i] == piece_id) {
      piece_ids_.erase(piece_ids_.begin() + i);
      break;
    }
  }
}

void Tile::AddNeighborId(int tile_id) {
  neighbor_ids_.push_back(tile_id);
}

std::vector<int> Tile::GetNeighborIds() const {
  return neighbor_ids_;
}

int Tile::GetId() const {
  return id_;
}

bool Tile::IsEmpty() const {
  return piece_ids_.empty();
}

BeliefState::BeliefState(std::vector<int> observers) : observers_(observers), current_player_(0), count_pieces_(0) {}

BeliefState::~BeliefState() {}

void BeliefState::InitializeTiles(const std::unordered_map<int, Tile>& tiles) {
  tiles_ = tiles;
}

void BeliefState::AddPiece(int owner, const std::vector<PieceType>& types, PieceType type, int tile_id) {
  pieces_.insert({count_pieces_, Piece(count_pieces_, owner, types, type, tile_id)});
  tiles_[tile_id].AddPieceId(count_pieces_);
  count_pieces_++;
}

std::unordered_map<int, Piece> BeliefState::GetPieces() const {
  return pieces_;
}

Piece BeliefState::GetPiece(int piece_id) const {
  return pieces_.at(piece_id);
}

std::unordered_map<int, Tile> BeliefState::GetTiles() const {
  return tiles_;
}

Tile BeliefState::GetTile(int tile_id) const {
  return tiles_.at(tile_id);
}

int BeliefState::GetCurrentPlayer() const {
  return current_player_;
}

void BeliefState::SetCurrentPlayer(int player) {
  current_player_ = player;
}

void BeliefState::MovePiece(int tile_0_id, int tile_0_idx, int tile_1_id, int tile_1_idx) {
  pieces_[tiles_[tile_0_id].GetPieceId(tile_0_idx)].SetTileId(tile_1_id);
  tiles_[tile_1_id].InsertPieceId(tiles_[tile_0_id].GetPieceId(tile_0_idx), tile_1_idx);
  tiles_[tile_0_id].RemovePieceIdFromIdx(tile_0_idx);
}

void BeliefState::RemovePiece(int piece_id) {
  int tile_id = pieces_[piece_id].GetTileId();
  tiles_[tile_id].RemovePieceId(piece_id);
  pieces_.erase(piece_id);
}

void BeliefState::Hide(const std::vector<int>& tile_ids, const std::vector<int>& observers) {

  int count_pieces = 0;
  std::unordered_map<PieceType, double> sum_probabilities;
  for (const auto& tile_id : tile_ids) {
    for (const auto& piece_id : tiles_[tile_id].GetPieceIds()) {
      if (std::find(observers.begin(), observers.end(), pieces_[piece_id].GetOwner()) != observers.end()) {
        continue;
      }
      for (const auto& [type, prob] : pieces_[piece_id].GetProbabilities()) {
        if (sum_probabilities.find(type) == sum_probabilities.end()) {
          sum_probabilities[type] = 0.0;
        }
        sum_probabilities[type] += prob;
      }
      count_pieces++;
    }
  }

  for (const auto& tile_id : tile_ids) {
    for (const auto& piece_id : tiles_[tile_id].GetPieceIds()) {
      if (std::find(observers.begin(), observers.end(), pieces_[piece_id].GetOwner()) != observers.end()) {
        continue;
      }
      std::unordered_map<PieceType, double> new_probabilities;
      for (const auto& [type, prob] : pieces_[piece_id].GetProbabilities()) {
        new_probabilities[type] = sum_probabilities[type] / count_pieces;
      }
      pieces_[piece_id].SetProbabilities(new_probabilities);
    }
  }

}

void BeliefState::UpdateBelief(int piece_id, PieceType piece_type, double p) {

  double delta = p - pieces_[piece_id].GetProbability(piece_type);

  if (std::abs(delta) < 1e-6) {
    return;
  }

  std::unordered_set<PieceType> updated_types;

  std::unordered_map<PieceType, double> new_probabilities_piece;
  double remaining_prob = 1.0 - pieces_[piece_id].GetProbability(piece_type);
  for (const auto& [type, prob] : pieces_[piece_id].GetProbabilities()) {
    if (type == piece_type) {
      new_probabilities_piece[piece_type] = p;
      continue;
    }
    new_probabilities_piece[type] = prob - delta * prob / remaining_prob;
    if (std::abs(new_probabilities_piece[type] - prob) > 1e-6) {
      updated_types.insert(type);
    }
  }

  double global_remaining_prob = 0.0;
  for (const auto& [id, piece] : pieces_) {
    if (piece.GetOwner() != pieces_[piece_id].GetOwner() || piece.GetId() == piece_id) {
      continue;
    }
    global_remaining_prob += piece.GetProbability(piece_type);
  }

  for (auto& [id, piece] : pieces_) {
    if (piece.GetOwner() != pieces_[piece_id].GetOwner() || piece.GetId() == piece_id) {
      continue;
    }

    double current_delta = - delta * piece.GetProbability(piece_type) / global_remaining_prob;
    
    if (std::abs(current_delta) < 1e-6) {
      continue;
    }

    double current_remaining_prob = 0.0;
    for (const auto& [type, prob] : piece.GetProbabilities()) {
      if (updated_types.find(type) != updated_types.end()) {
        current_remaining_prob += prob;
      }
    }

    std::unordered_map<PieceType, double> new_probabilities;
    for (const auto& [type, prob] : piece.GetProbabilities()) {
      if (type == piece_type) {
        new_probabilities[type] = prob + current_delta;
        continue;
      }
      if (updated_types.find(type) == updated_types.end()) {
        new_probabilities[type] = prob;
        continue;
      }
      new_probabilities[type] = prob - current_delta * prob / current_remaining_prob;
    }
    piece.SetProbabilities(new_probabilities);
  }

  pieces_[piece_id].SetProbabilities(new_probabilities_piece);
}

void BeliefState::print() const {
  
  std::cout << "Tiles: "  << std::endl;
  for (const auto& [id, tile] : tiles_) {
    std::cout << "    Tile " << id << ":" << std::endl;
    std::cout << "        Neighbors: ";
    for (const auto& neighbor_id : tile.GetNeighborIds()) {
      std::cout << neighbor_id << " ";
    }
    std::cout << std::endl;
    std::cout << "        Pieces: ";
    for (const auto& piece_id : tile.GetPieceIds()) {
      std::cout << piece_id << " ";
    }
    std::cout << std::endl;
  }
  std::cout << "Pieces: "  << std::endl;
  for (const auto& [id, piece] : pieces_) {
    std::cout << "    Piece " << id << ":" << std::endl;
    std::cout << "        Owner: " << piece.GetOwner() << std::endl;
    std::cout << "        Probabilities: ";
    for (const auto& [type, prob] : piece.GetProbabilities()) {
      std::cout << "(";
      for (const auto& v : type.values) {
        std::cout << v << ",";
      }
      std::cout << ")";
      std::cout << ": " << prob << ", ";
    }
    std::cout << std::endl;
  }

}

StochasticAction::StochasticAction(Action action, double p) : action(action), p(p) {}

StochasticTransition::StochasticTransition(BeliefState next_state, double p) : next_state(next_state), p(p) {}