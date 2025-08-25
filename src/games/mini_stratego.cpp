#include "Belief-SG/games/mini_stratego.h"

#include <cstddef>
#include <memory>
#include <random>
#include <vector>
#include <iostream>
#include <algorithm>

#include "Belief-SG/core/action.h"
#include "Belief-SG/core/moves/set_variable.h"
#include "Belief-SG/core/piece_type.h"
#include "Belief-SG/core/piece_value.h"
#include "Belief-SG/core/play_graph.h"
#include "Belief-SG/core/player_id.h"
#include "Belief-SG/core/point_of_view.h"
#include "Belief-SG/core/position.h"
#include "Belief-SG/core/state.h"
#include "Belief-SG/core/moves/move_piece.h"
#include "Belief-SG/core/moves/set_next_player.h"
#include "Belief-SG/core/moves/remove_piece_value.h"
#include "Belief-SG/core/moves/reveal.h"
#include "Belief-SG/core/variable.h"

namespace belief_sg {

BattleStratego::BattleStratego(const Position& from) : from_(from) {}

std::vector<ProbTransition> BattleStratego::apply(const State& state) const {
    if (state.get_pieces_at(from_).size() < 2) {
        return std::vector<ProbTransition>{ProbTransition({state, 1.0})};
    }

    auto survives = [](const PieceValue& attacker, const PieceValue& defender) {
        if (attacker == defender) {
            return false;
        }
        if (attacker == PieceValue({{"rank", "Flag"}})) {
            return false;
        }
        if (attacker == PieceValue({{"rank", "Bomb"}})) {
            return defender != PieceValue({{"rank", "Miner"}});
        }
        if (attacker == PieceValue({{"rank", "Miner"}})) {
            return defender != PieceValue({{"rank", "Soldier"}});
        }
        if (attacker == PieceValue({{"rank", "Soldier"}})) {
            return defender != PieceValue({{"rank", "Bomb"}});
        }
        return false;
    };

    std::vector<ProbTransition> transitions;

    std::vector<Piece> pieces = state.get_pieces_at(from_);
    for (const PieceValue& value_1 : pieces[0].values) {
        for (const PieceValue& value_2 : pieces[1].values) {
            State new_state(state);

            double transition_prob = pieces[0].probability(value_1) * pieces[1].probability(value_2);

            new_state.assign_piece_value(Position(from_.cell_id(), 0), value_1);
            new_state.assign_piece_value(Position(from_.cell_id(), 1), value_2);

            bool survives_1 = survives(value_1, value_2);
            bool survives_2 = survives(value_2, value_1);

            if (!survives_1 && !survives_2) {
                new_state.remove_piece(Position(from_.cell_id(), 0));
                new_state.remove_piece(Position(from_.cell_id(), 0));
            } else if (!survives_1) {
                new_state.remove_piece(Position(from_.cell_id(), 0));
            } else if (!survives_2) {
                new_state.remove_piece(Position(from_.cell_id(), 1));
            }

            transitions.push_back(ProbTransition({new_state, transition_prob}));
        }
    }

    return transitions;
}

void BattleStratego::apply_inplace(State& state, std::mt19937& generator) const {
    if (state.get_pieces_at(from_).size() < 2) {
        return;
    }

    auto survives = [](const PieceValue& attacker, const PieceValue& defender) {
        if (attacker == defender) {
            return false;
        }
        if (attacker == PieceValue({{"rank", "Flag"}})) {
            return false;
        }
        if (attacker == PieceValue({{"rank", "Bomb"}})) {
            return defender != PieceValue({{"rank", "Miner"}});
        }
        if (attacker == PieceValue({{"rank", "Miner"}})) {
            return defender != PieceValue({{"rank", "Soldier"}});
        }
        if (attacker == PieceValue({{"rank", "Soldier"}})) {
            return defender != PieceValue({{"rank", "Bomb"}});
        }
        return false;
    };

    std::vector<Piece> pieces = state.get_pieces_at(from_);

    std::uniform_int_distribution<std::size_t> dist_0(0, pieces[0].values.size()-1);
    PieceValue value_0 = pieces[0].values[dist_0(generator)];
    state.assign_piece_value(Position(from_.cell_id(), 0), value_0);

    std::uniform_int_distribution<std::size_t> dist_1(0, pieces[1].values.size()-1);
    PieceValue value_1 = pieces[1].values[dist_1(generator)];
    state.assign_piece_value(Position(from_.cell_id(), 1), value_1);

    bool survives_0 = survives(value_0, value_1);
    bool survives_1 = survives(value_1, value_0);

    if (!survives_0 && !survives_1) {
        state.remove_piece(Position(from_.cell_id(), 0));
        state.remove_piece(Position(from_.cell_id(), 0));
    } else if (!survives_0) {
        state.remove_piece(Position(from_.cell_id(), 0));
    } else if (!survives_1) {
        state.remove_piece(Position(from_.cell_id(), 1));
    }
}

std::unique_ptr<Move> BattleStratego::clone() const {
    return std::make_unique<BattleStratego>(from_);
}

bool BattleStratego::is_equals(const Move& other) const {
    return from_ == dynamic_cast<const BattleStratego&>(other).from_;
}

MiniStratego::MiniStratego() {
    std::vector<std::vector<int>> adj;
    const int dim = 5;
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            std::vector<int> neighbors;
            if (i > 0) {
                neighbors.push_back((i-1)*dim + j);
            }
            if (i < dim-1) {
                neighbors.push_back((i+1)*dim + j);
            }
            if (j > 0) {
                neighbors.push_back(i*dim + j-1);
            }
            if (j < dim-1) {
                neighbors.push_back(i*dim + j+1);
            }
            adj.push_back(neighbors);
        }
    }
    adj.push_back(std::vector<int>{0, 1, 2, 3, 4});
    adj.push_back(std::vector<int>{20, 21, 22, 23, 24});
    play_graph_ = PlayGraph(adj);

    blue_stratego_type_ = std::make_shared<const PieceType>(std::vector<PieceValue>{
        PieceValue({{"rank", "Flag"}}),
        PieceValue({{"rank", "Bomb"}}),
        PieceValue({{"rank", "Miner"}}),
        PieceValue({{"rank", "Soldier"}})
    });
    red_stratego_type_ = std::make_shared<const PieceType>(std::vector<PieceValue>{
        PieceValue({{"rank", "Flag"}}),
        PieceValue({{"rank", "Bomb"}}),
        PieceValue({{"rank", "Miner"}}),
        PieceValue({{"rank", "Soldier"}})
    });
}

std::string MiniStratego::name() const {
    return "Mini Stratego";
}

int MiniStratego::num_players() const {
    return num_players_;
}

const PlayGraph& MiniStratego::play_graph() const {
    return play_graph_;
}

State MiniStratego::initial_state(const PointOfView& point_of_view) const {
    StateBuilder state_builder(this->shared_from_this(), point_of_view);

    state_builder.set_initial_players({0});

    state_builder.add_variable({"boring_moves", 0});

    state_builder.add_piece(blue_stratego_type_, PieceValue({{"rank", "Flag"}}), {0}, Position(25));
    state_builder.add_piece(blue_stratego_type_, PieceValue({{"rank", "Bomb"}}), {0}, Position(25));
    state_builder.add_piece(blue_stratego_type_, PieceValue({{"rank", "Miner"}}), {0}, Position(25));
    state_builder.add_piece(blue_stratego_type_, PieceValue({{"rank", "Soldier"}}), {0}, Position(25));
    state_builder.add_piece(blue_stratego_type_, PieceValue({{"rank", "Soldier"}}), {0}, Position(25));

    state_builder.add_piece(red_stratego_type_, PieceValue({{"rank", "Flag"}}), {1}, Position(26));
    state_builder.add_piece(red_stratego_type_, PieceValue({{"rank", "Bomb"}}), {1}, Position(26));
    state_builder.add_piece(red_stratego_type_, PieceValue({{"rank", "Miner"}}), {1}, Position(26));
    state_builder.add_piece(red_stratego_type_, PieceValue({{"rank", "Soldier"}}), {1}, Position(26));
    state_builder.add_piece(red_stratego_type_, PieceValue({{"rank", "Soldier"}}), {1}, Position(26));

    return state_builder.build();
}

std::vector<ProbAction> MiniStratego::legal_actions(const State& state, PlayerId player_id) const {
    if (std::ranges::find(state.current_players(), player_id) == state.current_players().end()) {
        std::cout << "Player is not current \n";
        return {};
    }

    std::vector<ProbAction> actions;
    if (!state.get_pieces_at(Position(25 + player_id)).empty()) {
        for (const Position& neighbor_position : play_graph_.get_neighbor_positions(Position(25 + player_id))) {
            if (!state.get_pieces_at(neighbor_position).empty()) {
                continue;
            }
            std::vector<std::unique_ptr<Move>> moves;
            moves.push_back(std::make_unique<MovePiece>(Position(25 + player_id), neighbor_position));
            if (state.get_pieces_at(Position(25 + player_id)).size() == 1) {
                moves.push_back(std::make_unique<SetNextPlayer>(1 - player_id));
            }
            actions.push_back(
                ProbAction({
                    .action = Action(std::move(moves)),
                    .probability = 1.0
                })
            );
        }
        return actions;
    }

    std::shared_ptr<const PieceType> current_type = player_id == 0 ? blue_stratego_type_ : red_stratego_type_;
    for (int i = 0; i < 25; i++) {
        if (state.get_pieces_at(Position(i)).empty()) {
            continue;
        }
        for (const Piece& piece : state.get_pieces_at(Position(i))) {
            if (piece.type != current_type) {
                continue;
            }
            if (!piece.can_be(PieceValue({{"rank", "Miner"}})) && !piece.can_be(PieceValue({{"rank", "Soldier"}}))) {
                continue;
            }

            double action_prob = piece.probability(PieceValue({{"rank", "Miner"}})) + piece.probability(PieceValue({{"rank", "Soldier"}}));

            for (const Position& neighbor_position : play_graph_.get_neighbor_positions(Position(i))) {
                if (state.get_pieces_at(neighbor_position).empty()) {
                    std::vector<std::unique_ptr<Move>> moves;
                    moves.push_back(std::make_unique<RemovePieceValue>(Position(i), PieceValue({{"rank", "Flag"}})));
                    moves.push_back(std::make_unique<RemovePieceValue>(Position(i), PieceValue({{"rank", "Bomb"}})));
                    moves.push_back(std::make_unique<MovePiece>(Position(i), neighbor_position));
                    moves.push_back(std::make_unique<SetVariable>(Variable("boring_moves", state.variable("boring_moves").value<int>()+1)));
                    moves.push_back(std::make_unique<SetNextPlayer>(1 - player_id));
                    actions.push_back(ProbAction({Action(std::move(moves)), action_prob}));
                } else if (state.get_pieces_at(neighbor_position).size() == 1 && state.get_pieces_at(neighbor_position)[0].type != current_type) {
                    std::vector<std::unique_ptr<Move>> moves;
                    moves.push_back(std::make_unique<RemovePieceValue>(Position(i), PieceValue({{"rank", "Flag"}})));
                    moves.push_back(std::make_unique<RemovePieceValue>(Position(i), PieceValue({{"rank", "Bomb"}})));
                    moves.push_back(std::make_unique<MovePiece>(Position(i), neighbor_position));
                    moves.push_back(std::make_unique<Reveal>(neighbor_position, std::vector<PlayerId>{0, 1}));
                    moves.push_back(std::make_unique<BattleStratego>(neighbor_position));
                    moves.push_back(std::make_unique<SetVariable>(Variable("boring_moves", 0)));
                    moves.push_back(std::make_unique<SetNextPlayer>(1 - player_id));
                    actions.push_back(ProbAction({Action(std::move(moves)), action_prob}));
                }
            }
        }
    }

    return actions;
}

bool MiniStratego::is_terminal(const State& state) const {
    if (state.variable("boring_moves").value<int>() >= 20) {
        return true;
    }

    bool red_flag = false;
    bool blue_flag = false;
    for (int i = 0; i < 27; i++) {
        for (const Piece& piece : state.get_pieces_at(Position(i))) {
            if (piece.type == blue_stratego_type_ && piece.can_be(PieceValue({{"rank", "Flag"}}))) {
                blue_flag = true;
            }
            if (piece.type == red_stratego_type_ && piece.can_be(PieceValue({{"rank", "Flag"}}))) {
                red_flag = true;
            }
            if (red_flag && blue_flag) {
                break;
            }
        }
    }

    std::vector<ProbAction> actions = legal_actions(state, state.current_players()[0]);

    return !red_flag || !blue_flag || actions.empty();
}

std::vector<double> MiniStratego::returns(const State& state) const {
    if (state.variable("boring_moves").value<int>() >= 20) {
        return {0.0, 0.0};
    }
    std::vector<ProbAction> actions = legal_actions(state, state.current_players()[0]);
    if (actions.empty()) {
        if (state.current_players()[0] == 0) {
            return {-1.0, 1.0};
        } else {
            return {1.0, -1.0};
        }
    }

    bool red_flag = false;
    bool blue_flag = false;
    for (int i = 0; i < 27; i++) {
        for (const Piece& piece : state.get_pieces_at(Position(i))) {
            if (piece.type == blue_stratego_type_ && piece.can_be(PieceValue({{"rank", "Flag"}}))) {
                blue_flag = true;
            }
            if (piece.type == red_stratego_type_ && piece.can_be(PieceValue({{"rank", "Flag"}}))) {
                red_flag = true;
            }
            if (red_flag && blue_flag) {
                break;
            }
        }
    }

    if (!red_flag) {
        return {-1.0, 1.0};
    }
    if (!blue_flag) {
        return {1.0, -1.0};
    }
    return {0.0, 0.0};
}

}  // namespace belief_sg
