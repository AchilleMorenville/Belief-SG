#ifndef BELIEF_SG_CORE_PLAY_GRAPH_H
#define BELIEF_SG_CORE_PLAY_GRAPH_H

#include "Belief-SG/core/position.h"
#include <vector>

namespace belief_sg {

struct PlayGraph {
public:
    PlayGraph() = default;
    explicit PlayGraph(const std::vector<std::vector<int>>& adjacency_list);

    [[nodiscard]] int size() const;

    [[nodiscard]] std::vector<Position> get_neighbor_positions(const Position& position) const;
private:
    std::vector<std::vector<int>> adjacency_list_;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_PLAY_GRAPH_H
