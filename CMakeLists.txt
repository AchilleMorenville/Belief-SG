cmake_minimum_required(VERSION 3.29)
project(Belief-SG)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(Belief-SG STATIC
    src/core/piece_attribute.cpp
    src/core/piece_value.cpp
    src/core/piece_type.cpp
    src/core/state.cpp
    src/core/play_graph.cpp
    src/core/piece_domain.cpp
    src/core/position.cpp
    src/core/variable.cpp
    src/core/point_of_view.cpp
    src/core/move.cpp
    src/core/action.cpp
    src/core/manager.cpp
    src/core/moves/move_piece.cpp
    src/core/moves/remove_piece.cpp
    src/core/moves/set_next_player.cpp
    src/core/moves/set_next_players.cpp
    src/core/moves/set_variable.cpp
    src/core/moves/remove_piece_value.cpp
    src/core/moves/remove_piece_values.cpp
    src/core/moves/reveal.cpp
    src/core/moves/shuffle.cpp
    src/core/game.cpp
    src/games/mini_stratego.cpp
    src/games/kuhn_poker.cpp
    src/games/goofspiel.cpp
    src/agents/random_agent.cpp
    src/agents/determinized_mc.cpp
    src/agents/determinized_uct.cpp
)

# ---------------- GECODE CONFIG ----------------
# Adjust these two paths depending on your system:
target_include_directories(Belief-SG PUBLIC /opt/homebrew/opt/gecode/include)
target_link_directories(Belief-SG PUBLIC /opt/homebrew/opt/gecode/lib)

target_link_libraries(Belief-SG PRIVATE
    gecodekernel
    gecodesupport
    gecodeint
    gecodesearch
)
