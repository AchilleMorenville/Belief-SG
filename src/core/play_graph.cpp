#include "Belief-SG/core/play_graph.h"
#include "Belief-SG/core/position.h"

#include <vector>

namespace belief_sg {

PlayGraph::PlayGraph(const std::vector<std::vector<int>>& adjacency_list) : adjacency_list_(adjacency_list) {}

int PlayGraph::size() const {
    return static_cast<int>(adjacency_list_.size());
}

std::vector<Position> PlayGraph::get_neighbor_positions(const Position& position) const {
    std::vector<Position> neighbor_positions;
    neighbor_positions.reserve(adjacency_list_[position.cell_id()].size());
    for (int neighbor : adjacency_list_[position.cell_id()]) {
        neighbor_positions.emplace_back(neighbor);
    }
    return neighbor_positions;
}

}  // namespace belief_sg
