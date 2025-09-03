#include "Belief-SG/games/cuckoo.h"

#include <memory>
#include <vector>
#include <iostream>

#include "Belief-SG/core/action.h"
#include "Belief-SG/core/moves/assign_piece_value.h"
#include "Belief-SG/core/moves/move_piece.h"
#include "Belief-SG/core/moves/remove_piece_values.h"
#include "Belief-SG/core/moves/remove_piece_value.h"
#include "Belief-SG/core/moves/reveal.h"
#include "Belief-SG/core/moves/set_next_player.h"
#include "Belief-SG/core/moves/set_next_players.h"
#include "Belief-SG/core/moves/set_variable.h"
#include "Belief-SG/core/moves/remove_piece.h"
#include "Belief-SG/core/moves/set_observers.h"

#include "Belief-SG/core/piece_attribute.h"
#include "Belief-SG/core/piece_type.h"
#include "Belief-SG/core/piece_value.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/position.h"
#include "Belief-SG/core/variable.h"
#include "Belief-SG/core/play_graph.h"

namespace belief_sg {

Cuckoo::Cuckoo(int num_players) : num_players_(num_players) {
    if (num_players_ < 3 || num_players_ > 26) {
        throw std::invalid_argument("Cuckoo must be played with 3 to 26 players");
    }

    // One nodes per player for the hand, one node per player for the spot where they play, one node for the deck
    std::vector<std::vector<int>> adj(num_players_ + 2);
    play_graph_ = PlayGraph(adj);

    std::vector<PieceValue> card_values;
    for (int rank_i = 1; rank_i <= 13; rank_i++) {
        card_values.push_back(PieceValue({{"rank", rank_i}}));
    }

    card_type_ = std::make_shared<const PieceType>(card_values);
}

std::string Cuckoo::name() const {
    return "Cuckoo";
}

int Cuckoo::num_players() const {
    return num_players_;
}

const PlayGraph& Cuckoo::play_graph() const {
    return play_graph_;
}

State Cuckoo::initial_state(const PointOfView& point_of_view) const {
    StateBuilder state_builder(this->shared_from_this(), point_of_view);
    
    state_builder.set_initial_players({kChancePlayerId});

    for (int rank_i = 1; rank_i <= 13; rank_i++) {
        for (int suit_i = 0; suit_i < 4; suit_i++) {  // Not used, only for multiplicity
            state_builder.add_piece(
                card_type_,
                PieceValue({{"rank", rank_i}, {"rank", rank_i}}),
                {}, 
                Position(num_players_)
            );
        }
    }

    state_builder.add_variable({"dealing", true});
    state_builder.add_variable({"exchange", false});  // If true, the previous player wants to switch cards
    state_builder.add_variable({"reveal", false});  // If true, everyone has to reveal a card
    state_builder.add_variable({"done", false});  // If true, everyone has revealed its card and the game is over

    return state_builder.build();
}

std::vector<ProbAction> Cuckoo::legal_actions(const State& state, PlayerId player_id) const {
    if (std::ranges::find(state.current_players(), player_id) == state.current_players().end()) {
        std::cout << "The player can't play in this state\n";
        return {};
    }

    std::vector<ProbAction> actions;

    if (player_id == kChancePlayerId) { // Dealing or removing cards
        bool dealing = state.variable("dealing").value<bool>();
        if (dealing) {
            int remaining = state.get_pieces_at(Position(num_players_)).size();
            int player_to = (52 - remaining) % num_players_;

            std::vector<std::unique_ptr<Move>> moves;
            moves.push_back(std::make_unique<MovePiece>(Position(num_players_), Position(player_to)));
            moves.push_back(std::make_unique<Reveal>(Position(player_to, (52 - remaining) / num_players_), std::vector<PlayerId>{player_to}));
            if (52 - num_players_ + 1 == remaining) { // Dealt to last player
                moves.push_back(std::make_unique<SetNextPlayer>(0));
                moves.push_back(std::make_unique<SetVariable>(Variable("dealing", false)));
            }

            actions.push_back(ProbAction(Action(std::move(moves)), 1.0));
        }
    } else {
        bool exchange = state.variable("exchange").value<bool>();
        bool reveal = state.variable("reveal").value<bool>();
        if (reveal) {
            std::vector<std::unique_ptr<Move>> moves;
            moves.push_back(std::make_unique<Reveal>(Position(player_id), all_players()));
            if (player_id == num_players_ - 1) {
                moves.push_back(std::make_unique<SetVariable>(Variable("reveal", false)));
                moves.push_back(std::make_unique<SetVariable>(Variable("done", true)));
                moves.push_back(std::make_unique<SetNextPlayers>(std::vector<PlayerId>{}));
            } else {
                moves.push_back(std::make_unique<SetNextPlayer>(player_id + 1));
            }
            actions.push_back(ProbAction(Action(std::move(moves)), 1.0));
        } else if (exchange) {
            // Previous player wants to exchange
            // If King -> not exchange
            if (state.get_piece_at(Position(player_id)).can_have(PieceAttribute("rank", 13))) {
                std::vector<std::unique_ptr<Move>> moves;
                moves.push_back(std::make_unique<SetVariable>(Variable("exchange", false)));
                moves.push_back(std::make_unique<AssignPieceValue>(Position(player_id), PieceValue({{"rank", 13}})));
                moves.push_back(std::make_unique<SetNextPlayer>(player_id));
                actions.push_back(ProbAction(Action(std::move(moves)), 1.0));
            }

            // If no King -> exchange
            if (state.get_piece_at(Position(player_id)).can_not_have(PieceAttribute("rank", 13))) {
                int previous_player_id = (player_id + num_players_ - 1) % num_players_;
                std::vector<std::unique_ptr<Move>> moves;
                moves.push_back(std::make_unique<SetVariable>(Variable("exchange", false)));
                moves.push_back(std::make_unique<RemovePieceValue>(Position(player_id), PieceValue({{"rank", 13}})));
                
                // Move cards
                moves.push_back(std::make_unique<MovePiece>(Position(player_id), Position(previous_player_id)));
                moves.push_back(std::make_unique<MovePiece>(Position(previous_player_id, 0), Position(player_id)));

                // Change Observers
                moves.push_back(std::make_unique<SetObservers>(Position(previous_player_id), std::vector<PlayerId>{previous_player_id}));
                moves.push_back(std::make_unique<SetObservers>(Position(player_id), std::vector<PlayerId>{player_id}));

                moves.push_back(std::make_unique<SetNextPlayer>(player_id));
                actions.push_back(ProbAction(Action(std::move(moves)), 1.0));
            }
        } else {
            // Regular play
            if (player_id < num_players_ - 1) {
                // Not the last player, can play normally
                // Either pass or want to exchange
                std::vector<std::unique_ptr<Move>> pass_moves;
                pass_moves.push_back(std::make_unique<SetVariable>(Variable("exchange", false)));
                pass_moves.push_back(std::make_unique<SetNextPlayer>((player_id + 1) % num_players_));
                actions.push_back(ProbAction(Action(std::move(pass_moves)), 1.0));

                std::vector<std::unique_ptr<Move>> exchange_moves;
                exchange_moves.push_back(std::make_unique<SetVariable>(Variable("exchange", true)));
                exchange_moves.push_back(std::make_unique<SetNextPlayer>((player_id + 1) % num_players_));
                actions.push_back(ProbAction(Action(std::move(exchange_moves)), 1.0));
            } else {
                if (state.get_pieces_at(Position(num_players_+1)).empty()) {  // Last player can exchange or pass
                    std::vector<std::unique_ptr<Move>> pass_moves;
                    pass_moves.push_back(std::make_unique<SetVariable>(Variable("exchange", false)));
                    pass_moves.push_back(std::make_unique<SetNextPlayer>((player_id + 1) % num_players_));  // Go back to first player
                    actions.push_back(ProbAction(Action(std::move(pass_moves)), 1.0));

                    std::vector<std::unique_ptr<Move>> reveal_top_deck_moves;
                    reveal_top_deck_moves.push_back(std::make_unique<MovePiece>(Position(num_players_), Position(num_players_+1)));
                    reveal_top_deck_moves.push_back(std::make_unique<Reveal>(Position(num_players_+1), all_players()));
                    actions.push_back(ProbAction(Action(std::move(reveal_top_deck_moves)), 1.0));
                } else {  // Last player exchange if no King

                    // If King don't change
                    if (state.get_piece_at(Position(num_players_+1)).can_have(PieceAttribute("rank", 13))) {
                        std::vector<std::unique_ptr<Move>> moves;
                        moves.push_back(std::make_unique<SetVariable>(Variable("exchange", false)));
                        moves.push_back(std::make_unique<SetVariable>(Variable("reveal", true)));
                        
                        moves.push_back(std::make_unique<SetNextPlayer>((player_id + 1) % num_players_));  // Go back to first player
                        actions.push_back(ProbAction(Action(std::move(moves)), 1.0));
                    }

                    // If no King -> exchange
                    if (state.get_piece_at(Position(num_players_+1)).can_not_have(PieceAttribute("rank", 13))) {
                        
                        std::vector<std::unique_ptr<Move>> moves;
                        moves.push_back(std::make_unique<SetVariable>(Variable("exchange", false)));
                        moves.push_back(std::make_unique<SetVariable>(Variable("reveal", true)));

                        // Move cards
                        moves.push_back(std::make_unique<MovePiece>(Position(player_id), Position(num_players_+1)));
                        moves.push_back(std::make_unique<MovePiece>(Position(num_players_+1, 0), Position(player_id)));

                        // Change Observers
                        moves.push_back(std::make_unique<Reveal>(Position(num_players_+1), all_players()));
                        moves.push_back(std::make_unique<SetObservers>(Position(player_id), std::vector<PlayerId>{player_id}));

                        moves.push_back(std::make_unique<SetNextPlayer>((player_id + 1) % num_players_));
                        
                        actions.push_back(ProbAction(Action(std::move(moves)), 1.0));
                    }
                }
            }
        }
    }

    return actions;
}

bool Cuckoo::is_terminal(const State& state) const {
    return state.variable("done").value<bool>();
}

std::vector<double> Cuckoo::returns(const State& state) const {
    if (!is_terminal(state)) {
        return std::vector<double>(num_players_, 0.0);
    }
    int min_rank = 14;
    int min_player_id = -1;
    for (int player_id = 0; player_id < num_players_; player_id++) {
        int rank = state.get_piece_at(Position(player_id)).values.back().get_attribute("rank").value<int>();
        if (rank < min_rank) {
            min_rank = rank;
            min_player_id = player_id;
        }
    }
    std::vector<double> returns(num_players_, 1.0);
    returns[min_player_id] = 0.0;

    return returns;
}

std::vector<PlayerId> Cuckoo::all_players() const {
    std::vector<PlayerId> players(num_players_);
    std::iota(players.begin(), players.end(), 0);
    return players;
}

}  // namespace belief_sg
