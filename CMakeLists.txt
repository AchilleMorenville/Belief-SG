cmake_minimum_required(VERSION 3.10)
project(Belief-SG)
set(CMAKE_CXX_STANDARD 20)

add_executable(Belief-SG src/main.cc src/game.cc src/stratego.cc src/random_agent.cc src/referee.cc)

target_include_directories(Belief-SG PUBLIC
                           "${PROJECT_SOURCE_DIR}/include"
                          )