#include "Belief-SG/games/kuhn_poker.h"

#include <vector>
#include <iostream>
#include <algorithm>
#include <memory>

#include "Belief-SG/core/action.h"
#include "Belief-SG/core/moves/reveal.h"
#include "Belief-SG/core/moves/set_variable.h"
#include "Belief-SG/core/piece_type.h"
#include "Belief-SG/core/piece_value.h"
#include "Belief-SG/core/play_graph.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/point_of_view.h"
#include "Belief-SG/core/position.h"
#include "Belief-SG/core/variable.h"

#include "Belief-SG/core/moves/move_piece.h"
#include "Belief-SG/core/moves/set_next_player.h"
#include "Belief-SG/core/moves/set_next_players.h"

namespace belief_sg {

KuhnPoker::KuhnPoker() {
    std::vector<std::vector<int>> adj(3, std::vector<int>());
    play_graph_ = PlayGraph(adj);

    card_type_ = std::make_shared<const PieceType>(std::vector<PieceValue>{
        PieceValue({{"rank", "J"}}),
        PieceValue({{"rank", "Q"}}),
        PieceValue({{"rank", "K"}})
    });
}

std::string KuhnPoker::name() const {
    return "Kuhn Poker";
}

int KuhnPoker::num_players() const {
    return num_players_;
}

const PlayGraph& KuhnPoker::play_graph() const {
    return play_graph_;
}

State KuhnPoker::initial_state(const PointOfView& point_of_view) const {
    StateBuilder state_builder(this->shared_from_this(), point_of_view);

    state_builder.set_initial_players({kChancePlayerId});

    state_builder.add_piece(card_type_, PieceValue({{"rank", "J"}}), {}, Position(0));
    state_builder.add_piece(card_type_, PieceValue({{"rank", "Q"}}), {}, Position(0));
    state_builder.add_piece(card_type_, PieceValue({{"rank", "K"}}), {}, Position(0));

    state_builder.add_variable({"pot", 2});
    state_builder.add_variable({"players_money", std::vector<int>{-1, -1}});

    state_builder.add_variable({"first_better", kInvalidPlayerId});

    return state_builder.build();
}

std::vector<ProbAction> KuhnPoker::legal_actions(const State& state, PlayerId player_id) const {
    if (std::ranges::find(state.current_players(), player_id) == state.current_players().end()) {
        std::cout << "Player is not current \n";
        return {};
    }

    std::vector<ProbAction> actions;
    if (player_id == kChancePlayerId) {  // Deal cards
        std::vector<std::unique_ptr<Move>> moves;

        int player = 3 - state.get_pieces_at(Position(0)).size();

        moves.push_back(std::make_unique<MovePiece>(Position(0), Position(player+1)));
        moves.push_back(std::make_unique<Reveal>(Position(player+1), std::vector<PlayerId>{player}));
        if (player == 1) {
            moves.push_back(std::make_unique<SetNextPlayer>(0));
        }
        actions.push_back(ProbAction(Action(std::move(moves)), 1.0));
    } else {
        int first_better = state.variable("first_better").value<int>();
        if (first_better == kInvalidPlayerId) {  // Previous player checked or first player
            // Check
            std::vector<std::unique_ptr<Move>> moves_check;
            if (player_id == 1) {
                moves_check.push_back(std::make_unique<Reveal>(Position(1), std::vector<PlayerId>{0, 1}));
                moves_check.push_back(std::make_unique<Reveal>(Position(2), std::vector<PlayerId>{0, 1}));
                moves_check.push_back(std::make_unique<SetNextPlayers>(std::vector<PlayerId>{}));
            } else {
                moves_check.push_back(std::make_unique<SetNextPlayer>(1 - player_id));
            }
            actions.push_back(ProbAction(Action(std::move(moves_check)), 1.0));
            // Bet
            std::vector<std::unique_ptr<Move>> moves_bet;
            moves_bet.push_back(std::make_unique<SetVariable>(Variable("first_better", player_id)));
            moves_bet.push_back(std::make_unique<SetVariable>(Variable("pot", state.variable("pot").value<int>() + 1)));

            auto players_money = state.variable("players_money").value<std::vector<int>>();
            players_money[player_id] -= 1;
            moves_bet.push_back(std::make_unique<SetVariable>(Variable("players_money", players_money)));
            moves_bet.push_back(std::make_unique<SetNextPlayer>(1 - player_id));
            actions.push_back(ProbAction(Action(std::move(moves_bet)), 1.0));
        } else {
            // Call
            std::vector<std::unique_ptr<Move>> moves_call;
            moves_call.push_back(std::make_unique<SetVariable>(Variable("pot", state.variable("pot").value<int>() + 1)));

            auto players_money = state.variable("players_money").value<std::vector<int>>();
            players_money[player_id] -= 1;
            moves_call.push_back(std::make_unique<SetVariable>(Variable("players_money", players_money)));
            moves_call.push_back(std::make_unique<Reveal>(Position(1), std::vector<PlayerId>{0, 1}));
            moves_call.push_back(std::make_unique<Reveal>(Position(2), std::vector<PlayerId>{0, 1}));
            moves_call.push_back(std::make_unique<SetNextPlayers>(std::vector<PlayerId>{}));
            actions.push_back(ProbAction(Action(std::move(moves_call)), 1.0));
            // Fold
            std::vector<std::unique_ptr<Move>> moves_fold;
            moves_fold.push_back(std::make_unique<Reveal>(Position(1), std::vector<PlayerId>{0, 1}));
            moves_fold.push_back(std::make_unique<Reveal>(Position(2), std::vector<PlayerId>{0, 1}));
            moves_fold.push_back(std::make_unique<SetNextPlayers>(std::vector<PlayerId>{}));
            actions.push_back(ProbAction(Action(std::move(moves_fold)), 1.0));
        }
    }
    return actions;
}

bool KuhnPoker::is_terminal(const State& state) const {
    return state.current_players().empty();
}

std::vector<double> KuhnPoker::returns(const State& state) const {
    if (!state.current_players().empty()) {
        return {0.0, 0.0};
    }

    auto players_money = state.variable("players_money").value<std::vector<int>>();
    auto pot = state.variable("pot").value<int>();
    auto first_better = state.variable("first_better").value<int>();

    if (first_better == kInvalidPlayerId) {
        if (wins(state.get_piece_at(Position(1)).values[0], state.get_piece_at(Position(2)).values[0])) {
            players_money[0] += pot;
        } else {
            players_money[1] += pot;
        }
        return std::vector<double>{
            static_cast<double>(players_money[0]),
            static_cast<double>(players_money[1])
        };
    } else {
        if (pot == 4) {
            if (wins(state.get_piece_at(Position(1)).values[0], state.get_piece_at(Position(2)).values[0])) {
                players_money[0] += pot;
            } else {
                players_money[1] += pot;
            }
        } else {
            players_money[first_better] += pot;
        }
        return std::vector<double>{
            static_cast<double>(players_money[0]),
            static_cast<double>(players_money[1])
        };
    }
    return {0.0, 0.0};
}

bool KuhnPoker::wins(const PieceValue& value_first, const PieceValue& value_second) {
    if (value_first == PieceValue({{"rank", "K"}})) {
        return true;
    }
    if (value_first == PieceValue({{"rank", "Q"}})) {
        return value_second == PieceValue({{"rank", "J"}});
    }
    return false;
}

}
