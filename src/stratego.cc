#include "stratego.h"

#include <iostream>
#include <cmath>
#include <random>

StrategoGame::StrategoGame() {
  for (int i = 0; i < 4; i++) {
    piece_types_.push_back(PieceType({i}));
  }
  CreateDefaultTiles();
}

StrategoGame::~StrategoGame() {}

BeliefState StrategoGame::InitialState(std::vector<int> observers) const {
  
  BeliefState belief_state(observers);
  belief_state.InitializeTiles(default_tiles_);
  for (int i = 0; i < 2; i++) {
    belief_state.AddPiece(i, piece_types_, PieceType({FLAG}), hands_[i]);
    belief_state.AddPiece(i, piece_types_, PieceType({BOMB}), hands_[i]);
    belief_state.AddPiece(i, piece_types_, PieceType({MINER}), hands_[i]);
    for (int j = 0; j < 2; j++) {
      belief_state.AddPiece(i, piece_types_, PieceType({SOLDIER}), hands_[i]);
    }
  }
  belief_state.SetCurrentPlayer(0);
  for (int i = 0; i < 2; i++) {
    belief_state.Hide({hands_[i]}, observers);
  }
  return std::move(belief_state);
}

std::vector<StochasticAction> StrategoGame::LegalActions(const BeliefState& state, int player) const {
  std::vector<StochasticAction> actions;
  if (state.GetCurrentPlayer() != player) {
    return actions;
  }
  std::unordered_map<int, Tile> tiles = state.GetTiles();
  if (!tiles[hands_[player]].IsEmpty()) {
    for (const auto& neighbor_id : tiles[hands_[player]].GetNeighborIds()) {
      if (!tiles[neighbor_id].IsEmpty()) {
        continue;
      }
      actions.push_back(StochasticAction(EncodeAction(hands_[player], neighbor_id), 1.0));
    }
  } else {
    std::unordered_map<int, Piece> pieces = state.GetPieces();
    for (const auto& [id, piece] : pieces) {
      if (piece.GetOwner() == state.GetCurrentPlayer()) {
        for (const auto& neighbor_id : tiles[piece.GetTileId()].GetNeighborIds()) {
          if (!tiles[neighbor_id].IsEmpty() && pieces[tiles[neighbor_id].GetPieceIds().back()].GetOwner() == state.GetCurrentPlayer()) {
            continue;
          }
          double prob = piece.GetProbabilities().at(PieceType({MINER})) + piece.GetProbabilities().at(PieceType({SOLDIER}));
          if (prob > 1e-6) {
            actions.push_back(StochasticAction(EncodeAction(piece.GetTileId(), neighbor_id), prob));
          }
        }
      }
    }
  }
  return actions;
}

std::vector<StochasticTransition> StrategoGame::ApplyAction(const BeliefState& state, Action action) const  {
  std::vector<StochasticTransition> transitions;
  auto [pos_0, pos_1] = DecodeAction(action);
  
  if (pos_0 == hands_[state.GetCurrentPlayer()]) {
    std::unordered_map<int, Tile> tiles = state.GetTiles();

    BeliefState next_state = BeliefState(state);

    next_state.MovePiece(pos_0, tiles[pos_0].GetPieceIds().size()-1, pos_1, 0);
    if (tiles[pos_0].GetPieceIds().size() == 1) {
      next_state.SetCurrentPlayer((next_state.GetCurrentPlayer() + 1) % 2);
    }
    transitions.emplace_back(next_state, 1.0);
  } else {
    BeliefState copy_state = BeliefState(state);
    std::unordered_map<int, Tile> tiles = copy_state.GetTiles();
    std::unordered_map<int, Piece> pieces = copy_state.GetPieces();
    
    int piece_id_0 = tiles[pos_0].GetPieceIds().back();
    
    copy_state.UpdateBelief(piece_id_0, PieceType({FLAG}), 0.0);
    copy_state.UpdateBelief(piece_id_0, PieceType({BOMB}), 0.0);

    if (tiles[pos_1].IsEmpty()) {
      copy_state.MovePiece(pos_0, tiles[pos_0].GetPieceIds().size()-1, pos_1, 0);
      copy_state.SetCurrentPlayer((copy_state.GetCurrentPlayer() + 1) % 2);
      transitions.emplace_back(std::move(copy_state), 1.0);
    } else {
      int piece_id_1 = tiles[pos_1].GetPieceIds().back();
      for (const auto& [type_0, prob_0] : pieces[piece_id_0].GetProbabilities()) {
        if (std::abs(prob_0) < 1e-6) {
          continue;
        }
        for (const auto& [type_1, prob_1] : pieces[piece_id_1].GetProbabilities()) {
          if (std::abs(prob_1) < 1e-6) {
            continue;
          }
          BeliefState next_state = BeliefState(copy_state);
          double total_prob = prob_0 * prob_1;

          next_state.UpdateBelief(piece_id_0, type_0, 1.0);
          next_state.UpdateBelief(piece_id_1, type_1, 1.0);
          next_state.MovePiece(pos_0, tiles[pos_0].GetPieceIds().size()-1, pos_1, 0);

          if (Dies(type_0, type_1)) {
            next_state.RemovePiece(piece_id_0);
          }
          if (Dies(type_1, type_0)) {
            next_state.RemovePiece(piece_id_1);
          }
          transitions.emplace_back(next_state, total_prob);
        }
      }
    }
  }
  return transitions;
}

bool StrategoGame::IsTerminal(const BeliefState& state) const {
  std::unordered_map<int, Piece> pieces = state.GetPieces();
  double prob_flag_0 = 0.0;
  double prob_flag_1 = 0.0;
  
  for (const auto& [id, piece] : pieces) {
    if (piece.GetProbability(PieceType({FLAG})) > 0.0) {
      if (piece.GetOwner() == 0) {
        prob_flag_0 += piece.GetProbability(PieceType({FLAG}));
      } else {
        prob_flag_1 += piece.GetProbability(PieceType({FLAG}));
      }
    }
  }

  std::vector<StochasticAction> actions = LegalActions(state, state.GetCurrentPlayer());

  return std::abs(prob_flag_0) < 1e-6 || std::abs(prob_flag_1) < 1e-6 || actions.size() == 0;
}

std::vector<double> StrategoGame::Returns(const BeliefState& state) const {
  
  std::vector<StochasticAction> actions = LegalActions(state, state.GetCurrentPlayer());
  if (actions.size() == 0) {
    if (state.GetCurrentPlayer() == 0) {
      return {-1.0, 1.0};
    } else {
      return {1.0, -1.0};
    }
  }
  
  std::unordered_map<int, Piece> pieces = state.GetPieces();
  double prob_flag_0 = 0.0;
  double prob_flag_1 = 0.0;
  
  for (const auto& [id, piece] : pieces) {
    if (piece.GetProbability(PieceType({FLAG})) > 0.0) {
      if (piece.GetOwner() == 0) {
        prob_flag_0 += piece.GetProbability(PieceType({FLAG}));
      } else {
        prob_flag_1 += piece.GetProbability(PieceType({FLAG}));
      }
    }
  }
  if (std::abs(prob_flag_0) < 1e-6) {
    return {-1.0, 1.0};
  } else if (std::abs(prob_flag_1) < 1e-6) {
    return {1.0, -1.0};
  } else {
    return {0.0, 0.0};
  }
}

void StrategoGame::print(const BeliefState& state) const {
  
  std::unordered_map<int, Tile> tiles = state.GetTiles();
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 5; j++) {
      std::vector<int> piece_ids = tiles[i * 5 + j].GetPieceIds();
      if (piece_ids.size() > 0) {
        std::cout << piece_ids.back() << " ";
      } else {
        std::cout << ". ";
      }
    }
    std::cout << std::endl;
  }

  std::cout << std::endl;

  for (const auto& [id, piece] : state.GetPieces()) {
    std::cout << id << "-> ";
    for (const auto& [type, prob] : piece.GetProbabilities()) {
      std::cout << type.values[0] << ":" << prob << " ";
    }
    std::cout << std::endl;
  }

}

void StrategoGame::CreateDefaultTiles() {
  int id = 0;
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 5; j++) {
      default_tiles_.insert({id, Tile(id)});
      board_.push_back(id);
      id++;
    }
  }

  default_tiles_.insert({id, Tile(id)});
  id++;
  default_tiles_.insert({id, Tile(id)});
  id++;
  hands_ = { id-2, id-1 };

  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 5; j++) {
      if (i > 0) {
        default_tiles_[i * 5 + j].AddNeighborId((i - 1) * 5 + j);
      }
      if (i < 4) {
        default_tiles_[i * 5 + j].AddNeighborId((i + 1) * 5 + j);
      }
      if (j > 0) {
        default_tiles_[i * 5 + j].AddNeighborId(i * 5 + j - 1);
      }
      if (j < 4) {
        default_tiles_[i * 5 + j].AddNeighborId(i * 5 + j + 1);
      }
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 5; j++) {
      default_tiles_[hands_[i]].AddNeighborId(i * 20 + j);
    }
  }
}

bool StrategoGame::Dies(PieceType attacker, PieceType defender) const {
  if (attacker == defender) {
    return true;
  } else if (attacker == PieceType({FLAG})) {
    return true;
  } else if (attacker == PieceType({BOMB})) {
    return defender == PieceType({MINER});
  } else if (attacker == PieceType({MINER})) {
    return !(defender == PieceType({BOMB}));
  } else if (attacker == PieceType({SOLDIER})) {
    return defender == PieceType({BOMB});
  }
  return false;
}

int64_t StrategoGame::EncodeAction(int pos_0, int pos_1) const {
  return (int64_t) (pos_0 * 100 + pos_1);
}

std::pair<int, int> StrategoGame::DecodeAction(int64_t action) const {
  return std::make_pair(action / 100, action % 100);
}