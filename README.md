# Belief Stochastic Game (Belief-SG)

This repository provides a lightweight C++ framework for modeling imperfect-information games designed to support the development of domain-independent agents for General Game Playing (GGP). The framework introduces the **Belief Stochastic Game model (Belief-SG)**, a novel approach that shifts state estimation from the agent to the game model, allowing agents to focus purely on strategy development. It includes essential abstractions for game representation, agent behavior, and game management.

## Features

- **Game Abstraction**: Core interface for creating and managing game states.
- **Agent Interface**: Provides a base for creating various agent types with customizable behaviors.
- **Available Agents:**:
    - **Random Agent**
    - **Pure Monte Carlo Agent**
    - **Decoupled UCT Agent**
- **Available Games:**:
    - **Kuhn Poker**
    - **Mini-Stratego:** An example implementation that showcases the framework's extensibility for specific games.
    - **Goofspiel**
- **Game Manager**: Runs and enforces the game, managing agents, turns, and outcomes.

## Getting Started

### Prerequisites

- C++20 compatible compiler
- CMake (for building the project)
- [Gecode](https://github.com/Gecode/gecode?tab=readme-ov-file) (C++ constraint programming library)

> **Note:** You must modify the `CMakeLists.txt` file to point to your local Gecode installation paths for `include` and `lib`.

### Building the Framework

1. Clone the repository

2. Build using CMake:

   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

### Usage

1. **Initialize the Game**: Create an instance of a game by instantiating a `Game` object.
2. **Create Agents**: Define agents, such as `RandomAgent`, and specify their roles in the game.
3. **Run the Game with Manager**: Use the `Manager` class to manage game flow, enforce rules, and display results.

### Example Code

Below is an example code snippet showing how to set up and run a game:

```cpp
#include "Belief-SG/core/game.h"
#include "Belief-SG/core/agent.h"
#include "Belief-SG/core/manager.h"

#include "Belief-SG/games/kuhn_poker.h"

#include "Belief-SG/agents/random_agent.h"

int main() {

    // Create a game instance
    std::shared_ptr<belief_sg::Game> game = std::make_shared<belief_sg::KuhnPoker>();

    // Create agents
    std::vector<std::unique_ptr<belief_sg::Agent>> agents;
    agents.reserve(game->num_players());
    agents.push_back(std::make_unique<belief_sg::RandomAgent>());
    agents.push_back(std::make_unique<belief_sg::RandomAgent>());

    // Create game manager
    belief_sg::Manager manager(game, std::move(agents));

    // Run the game
    std::vector<double> scores = manager.play(true);

    return 0;
}
```

### Customizing the Framework

To implement a new game or agent:
- Add a new class in the `/games` directory and implement the specific game rules by deriving from the `Game` class.
- Add a new class in the `/agents` directory and extend the `Agent` class to implement custom decision-making logic.

## Contributing

Contributions are welcome! Please fork the repository and create a pull request with your changes.

## License

This project is licensed under the MIT License. See `LICENSE` for more details.
