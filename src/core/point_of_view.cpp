#include "Belief-SG/core/point_of_view.h"

#include <stdexcept>

#include "Belief-SG/core/player_id.h"

namespace belief_sg {

PointOfView::PointOfView(Type type) : type(type) {
    if (type == Type::Private) {
        throw std::invalid_argument("Private point of view must have a player id");
    }
}

PointOfView::PointOfView(Type type, PlayerId player_id) : type(type), player_id(player_id) {
    if (type == Type::Public || type == Type::World) {
        throw std::invalid_argument("Public point of view must not have a player id");
    }
}

std::string PointOfView::to_string() const {
    switch (type) {
        case Type::World:
            return "World";
        case Type::Public:
            return "Public";
        case Type::Private:
            return "Private(" + std::to_string(player_id) + ")";
    }
    return "";
}

}  // namespace belief_sg
