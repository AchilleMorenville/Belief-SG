#ifndef BELIEF_SG_CORE_POINT_OF_VIEW_H
#define BELIEF_SG_CORE_POINT_OF_VIEW_H

#include <cstdint>
#include <string>

#include "Belief-SG/core/player_id.h"

namespace belief_sg {

struct PointOfView {
    enum class Type : std::uint8_t {
        World,
        Public,
        Private
    };

    Type type{};
    PlayerId player_id{};

    explicit PointOfView(Type type);
    PointOfView(Type type, PlayerId player_id);

    PointOfView() = default;

    [[nodiscard]] std::string to_string() const;
};

}  // namespace belief_sg

#endif  //BELIEF_SG_CORE_POINT_OF_VIEW_H
