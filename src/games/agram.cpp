#include "Belief-SG/games/agram.h"

#include <memory>
#include <vector>
#include <iostream>

#include "Belief-SG/core/action.h"
#include "Belief-SG/core/moves/move_piece.h"
#include "Belief-SG/core/moves/remove_piece_values.h"
#include "Belief-SG/core/moves/reveal.h"
#include "Belief-SG/core/moves/set_next_player.h"
#include "Belief-SG/core/moves/set_next_players.h"
#include "Belief-SG/core/moves/set_variable.h"
#include "Belief-SG/core/moves/remove_piece.h"

#include "Belief-SG/core/piece_attribute.h"
#include "Belief-SG/core/piece_type.h"
#include "Belief-SG/core/piece_value.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/position.h"
#include "Belief-SG/core/variable.h"
#include "Belief-SG/core/play_graph.h"

namespace belief_sg {

Agram::Agram(int num_players) : num_players_(num_players) {
    if (num_players_ < 2 || num_players_ > 5) {
        throw std::invalid_argument("Agram must be played with 2 to 5 players");
    }

    // One nodes per player for the hand, one node per player for the spot where they play, one node for the deck
    std::vector<std::vector<int>> adj(2*num_players_ + 1);
    play_graph_ = PlayGraph(adj);

    std::vector<PieceValue> card_values;
    for (int suit_i = 0; suit_i < 4; suit_i++) {
        for (int rank_i = 3; rank_i <= (suit_i == 0 ? 10 : 11); rank_i++) {
            card_values.push_back(PieceValue({{"suit", suit_i}, {"rank", rank_i}}));
        }
    }

    card_type_ = std::make_shared<const PieceType>(card_values);
}

std::string Agram::name() const {
    return "Agram";
}

int Agram::num_players() const {
    return num_players_;
}

const PlayGraph& Agram::play_graph() const {
    return play_graph_;
}

State Agram::initial_state(const PointOfView& point_of_view) const {
    StateBuilder state_builder(this->shared_from_this(), point_of_view);
    
    state_builder.set_initial_players({kChancePlayerId});

    for (int piece_value_i = 0; piece_value_i < card_type_->size(); piece_value_i++) {
        state_builder.add_piece(
            card_type_,
            card_type_->value_from_index(piece_value_i),
            {}, 
            Position(2*num_players_)
        );
    }

    state_builder.add_variable({"dealing", true});
    state_builder.add_variable({"last_trick_winner", 0});

    return state_builder.build();
}

std::vector<ProbAction> Agram::legal_actions(const State& state, PlayerId player_id) const {
    if (std::ranges::find(state.current_players(), player_id) == state.current_players().end()) {
        std::cout << "The player can't play in this state\n";
        return {};
    }

    std::vector<ProbAction> actions;

    if (player_id == kChancePlayerId) { // Dealing or removing cards
        bool dealing = state.variable("dealing").value<bool>();
        if (dealing) {
            int remaining = state.get_pieces_at(Position(2*num_players_)).size();
            int player_to = (35 - remaining) % num_players_;

            std::vector<std::unique_ptr<Move>> moves;
            moves.push_back(std::make_unique<MovePiece>(Position(2*num_players_), Position(player_to)));
            moves.push_back(std::make_unique<Reveal>(Position(player_to, (35 - remaining) / num_players_), std::vector<PlayerId>{player_to}));
            if (35 - 6 * num_players_ + 1 == remaining) { // Dealt to last player
                moves.push_back(std::make_unique<SetNextPlayer>(0));
                moves.push_back(std::make_unique<SetVariable>(Variable("dealing", false)));
            }

            actions.push_back(ProbAction(Action(std::move(moves)), 1.0));
        } else {
            // Remove cards and check winner
            int last_trick_winner = state.variable("last_trick_winner").value<PlayerId>();
            int suit = state.get_pieces_at(Position(last_trick_winner+num_players_))[0].values[0].get_attribute("suit").value<int>();

            int best_rank = state.get_pieces_at(Position(last_trick_winner+num_players_))[0].values[0].get_attribute("suit").value<int>();
            int best_player = last_trick_winner;

            for (int i = 1; i < num_players_; i++) {
                int player = (last_trick_winner + i) % num_players_;
                int player_suit = state.get_pieces_at(Position(player+num_players_))[0].values[0].get_attribute("suit").value<int>();
                int player_rank = state.get_pieces_at(Position(player+num_players_))[0].values[0].get_attribute("rank").value<int>();
                if (player_suit == suit && player_rank > best_rank) {
                    best_rank = player_rank;
                    best_player = player;
                }
            }

            std::vector<std::unique_ptr<Move>> moves;
            moves.push_back(std::make_unique<SetVariable>(Variable("last_trick_winner", best_player)));
            for (int player = 0; player < num_players_; player++) {
                moves.push_back(std::make_unique<RemovePiece>(Position(player+num_players_)));
            }

            if (state.get_pieces_at(Position(0)).empty()) {
                moves.push_back(std::make_unique<SetNextPlayers>(std::vector<PlayerId>{}));
            } else {
                moves.push_back(std::make_unique<SetNextPlayer>(best_player));
            }
            actions.push_back(ProbAction(Action(std::move(moves)), 1.0));
        }
    } else {
        // Player's turn to play
        if (state.variable("last_trick_winner").value<PlayerId>() == player_id) {
            // Player won the last trick, they can play any card
            for (int stack_id = 0; stack_id < state.get_pieces_at(Position(player_id)).size(); stack_id++) {
                std::vector<std::unique_ptr<Move>> moves;
                moves.push_back(std::make_unique<MovePiece>(Position(player_id, stack_id), Position(player_id+num_players_)));
                moves.push_back(std::make_unique<Reveal>(Position(player_id+num_players_), all_players()));
                moves.push_back(std::make_unique<SetNextPlayer>((player_id + 1) % num_players_));
                actions.push_back(ProbAction(Action(std::move(moves)), 1.0));
            }
        } else {
            // Player did not win the last trick, they must play a card of the same suit. If they don't have one, they can play any card.
            
            int last_trick_winner = state.variable("last_trick_winner").value<PlayerId>();
            int suit = state.get_piece_at(Position(last_trick_winner + num_players_)).values[0].get_attribute("suit").value<int>();
            
            std::vector<PieceValue> follow_suit;
            std::vector<PieceValue> not_follow_suit;
            for (int suit_i = 0; suit_i < 4; suit_i++) {
                for (int rank_i = 3; rank_i <= (suit_i == 0 ? 10 : 11); rank_i++) {
                    if (suit_i == suit) {
                        follow_suit.push_back(PieceValue({{"suit", suit_i}, {"rank", rank_i}}));
                    } else {
                        not_follow_suit.push_back(PieceValue({{"suit", suit_i}, {"rank", rank_i}}));
                    }
                }
            }
            
            for (int stack_id = 0; stack_id < state.get_pieces_at(Position(player_id)).size(); stack_id++) {
                const Piece& piece = state.get_piece_at(Position(player_id, stack_id));
                if (!piece.can_have(PieceAttribute("suit", suit))) {
                    continue;
                }

                std::vector<std::unique_ptr<Move>> moves;
                moves.push_back(std::make_unique<MovePiece>(Position(player_id, stack_id), Position(player_id+num_players_)));
                
                // Remove not possible values
                moves.push_back(std::make_unique<RemovePieceValues>(Position(player_id+num_players_), not_follow_suit));
                
                moves.push_back(std::make_unique<Reveal>(Position(player_id+num_players_), all_players()));
                
                if ((player_id + 1) % num_players_ == last_trick_winner) {
                    // Next player is the last trick winner, we are at the end of the trick -> chance player
                    moves.push_back(std::make_unique<SetNextPlayer>(kChancePlayerId));
                } else {
                    moves.push_back(std::make_unique<SetNextPlayer>((player_id + 1) % num_players_));
                }
                
                actions.push_back(ProbAction(Action(std::move(moves)), 1.0));
            }

            // Check if state can be not played
            // Have to do it with constraints
            bool assignment_possible = state.assignment_possible(Position(player_id), follow_suit);
            if (assignment_possible) {
                // Can play cards that are not the suit
                for (int stack_id = 0; stack_id < state.get_pieces_at(Position(player_id)).size(); stack_id++) {
                    const Piece& piece = state.get_piece_at(Position(player_id, stack_id));
                    if (!piece.can_not_have(PieceAttribute("suit", suit))) {
                        continue;
                    }

                    std::vector<std::unique_ptr<Move>> moves;
                    moves.push_back(std::make_unique<RemovePieceValues>(Position(player_id), follow_suit));
                    moves.push_back(std::make_unique<MovePiece>(Position(player_id, stack_id), Position(player_id+num_players_)));
                    moves.push_back(std::make_unique<Reveal>(Position(player_id+num_players_), all_players()));
                    
                    if ((player_id + 1) % num_players_ == last_trick_winner) {
                        // Next player is the last trick winner, we are at the end of the trick -> chance player
                        moves.push_back(std::make_unique<SetNextPlayer>(kChancePlayerId));
                    } else {
                        moves.push_back(std::make_unique<SetNextPlayer>((player_id + 1) % num_players_));
                    }
                    
                    actions.push_back(ProbAction(Action(std::move(moves)), 1.0));
                }
            }
        }
    }

    return actions;
}

bool Agram::is_terminal(const State& state) const {
    return state.current_players().empty();
}

std::vector<double> Agram::returns(const State& state) const {
    if (!is_terminal(state)) {
        return std::vector<double>(num_players_, 0.0);
    }
    std::vector<double> scores = std::vector<double>(num_players_, 0.0);
    scores[state.variable("last_trick_winner").value<PlayerId>()]++;
    return scores;
}

std::vector<PlayerId> Agram::all_players() const {
    std::vector<PlayerId> players(num_players_);
    std::iota(players.begin(), players.end(), 0);
    return players;
}

}  // namespace belief_sg
