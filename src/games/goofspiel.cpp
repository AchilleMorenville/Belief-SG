#include "Belief-SG/games/goofspiel.h"

#include <vector>
#include <memory>
#include <numeric>
#include <iostream>

#include "Belief-SG/core/moves/move_piece.h"
#include "Belief-SG/core/moves/remove_piece.h"
#include "Belief-SG/core/moves/reveal.h"
#include "Belief-SG/core/moves/set_next_players.h"
#include "Belief-SG/core/moves/set_variable.h"
#include "Belief-SG/core/piece_type.h"
#include "Belief-SG/core/piece_value.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/position.h"
#include "Belief-SG/core/variable.h"

namespace belief_sg {

Goofspiel::Goofspiel(int num_players) : num_players_(num_players) {
    if (num_players_ < 2) {
        throw std::invalid_argument("Goofspiel must be played with at least 2 players");
    }

    std::vector<std::vector<int>> adj(2*num_players_ + 2);
    play_graph_ = PlayGraph(adj);

    std::vector<PieceValue> card_values;
    for (int i = 1; i <= 13; i++) {
        card_values.push_back(PieceValue({{"rank", i}}));
    }

    for (int i = 0; i < num_players_+1; i++) {
        card_types_.push_back(std::make_shared<const PieceType>(card_values));
    }
}

std::string Goofspiel::name() const {
    return "Goofspiel";
}

int Goofspiel::num_players() const {
    return num_players_;
}

const PlayGraph& Goofspiel::play_graph() const {
    return play_graph_;
}

State Goofspiel::initial_state(const PointOfView& point_of_view) const {
    StateBuilder state_builder(this->shared_from_this(), point_of_view);

    state_builder.set_initial_players({kChancePlayerId});

    for (PlayerId player_id = 0; player_id < num_players_; player_id++) {
        for (int rank = 1; rank <= 13; rank++) {
            state_builder.add_piece(card_types_[player_id], PieceValue({{"rank", rank}}), {player_id}, Position(player_id));
        }
    }

    for (int rank = 1; rank <= 13; rank++) {
        state_builder.add_piece(card_types_[num_players_], PieceValue({{"rank", rank}}), {}, Position(2*num_players_));
    }

    state_builder.add_variable({"scores", std::vector<double>(num_players_, 0.0)});

    return state_builder.build();
}

std::vector<ProbAction> Goofspiel::legal_actions(const State& state, PlayerId player_id) const {
    if (std::ranges::find(state.current_players(), player_id) == state.current_players().end()) {
        std::cout << "Player is not current \n";
        return {};
    }

    std::vector<ProbAction> actions;
    if (player_id == kChancePlayerId) {
        if (state.get_pieces_at(Position(2*num_players_+1)).empty()) {  // First turn
            std::vector<std::unique_ptr<Move>> moves;
            moves.push_back(std::make_unique<MovePiece>(Position(2*num_players_), Position(2*num_players_+1)));
            moves.push_back(std::make_unique<Reveal>(Position(2*num_players_+1, 0), all_players()));
            moves.push_back(std::make_unique<SetNextPlayers>(all_players()));
            actions.push_back(ProbAction(Action(std::move(moves)), 1.0));
        } else {  // Regular turn
            std::vector<std::unique_ptr<Move>> moves;
            int max_rank = -1;
            std::vector<PlayerId> max_players;
            for (int cell_id = num_players_; cell_id < 2*num_players_; cell_id++) {
                const Piece& piece = state.get_piece_at(Position(cell_id));
                if (piece.values[0].get_attribute("rank").value<int>() > max_rank) {
                    max_rank = piece.values[0].get_attribute("rank").value<int>();
                    max_players = { cell_id-num_players_ };
                } else if (piece.values[0].get_attribute("rank").value<int>() == max_rank) {
                    max_players.push_back(cell_id-num_players_);
                }
                moves.push_back(std::make_unique<RemovePiece>(Position(cell_id)));
            }

            int piece_rank = state.get_piece_at(Position(2*num_players_+1)).values[0].get_attribute("rank").value<int>();

            auto scores = state.variable("scores").value<std::vector<double>>();
            for (PlayerId max_player : max_players) {
                scores[max_player] += (1.0 * piece_rank) / max_players.size();
            }
            moves.push_back(std::make_unique<SetVariable>(Variable("scores", scores)));
            moves.push_back(std::make_unique<RemovePiece>(Position(2*num_players_+1)));
            if (state.get_pieces_at(Position(2*num_players_)).empty()) {
                moves.push_back(std::make_unique<SetNextPlayers>(std::vector<PlayerId>{}));
            } else {
                moves.push_back(std::make_unique<MovePiece>(Position(2*num_players_), Position(2*num_players_+1)));
                moves.push_back(std::make_unique<Reveal>(Position(2*num_players_+1), all_players()));
                moves.push_back(std::make_unique<SetNextPlayers>(all_players()));
            }
            actions.push_back(ProbAction(Action(std::move(moves)), 1.0));
        }
    } else {
        for (int stack_id = 0; stack_id < state.get_pieces_at(Position(player_id)).size(); stack_id++) {
            std::vector<std::unique_ptr<Move>> moves;
            moves.push_back(std::make_unique<MovePiece>(Position(player_id, stack_id), Position(num_players_+player_id)));
            moves.push_back(std::make_unique<Reveal>(Position(player_id+num_players_), all_players()));
            moves.push_back(std::make_unique<SetNextPlayers>(std::vector<PlayerId>{kChancePlayerId}));
            actions.push_back(ProbAction(Action(std::move(moves)), 1.0));
        }
    }

    return actions;
}

bool Goofspiel::is_terminal(const State& state) const {
    return state.current_players().empty();
}

std::vector<double> Goofspiel::returns(const State& state) const {
    if (!state.current_players().empty()) {
        return std::vector<double>(num_players_, 0.0);
    }
    return state.variable("scores").value<std::vector<double>>();
}

std::vector<PlayerId> Goofspiel::all_players() const {
    std::vector<PlayerId> players(num_players_);
    std::iota(players.begin(), players.end(), 0);
    return players;
}

}  // namespace belief_sg
