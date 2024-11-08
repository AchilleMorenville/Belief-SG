# Belief Stochastic Game (Belief-SG)

This repository provides a lightweight C++ framework for modeling imperfect-information games designed to support the development of domain-independent agents for General Game Playing (GGP). The framework introduces the **Belief Stochastic Game model (Belief-SG)**, a novel approach that shifts state estimation from the agent to the game model, allowing agents to focus purely on strategy development. It includes essential abstractions for game representation, agent behavior, and game management.

## Features

- **Game Abstraction**: Core interface for creating and managing game states.
- **Agent Interface**: Provides a base for creating various agent types with customizable behaviors.
- **Random Agent**: Example of a stochastic agent that makes decisions based on random selections.
- **Referee**: Manages and enforces game rules, tracking and determining game outcomes.
- **Sample Game (MiniStratego)**: An example implementation that showcases the framework's extensibility for specific games.

## Getting Started

### Prerequisites

- C++20 compatible compiler
- CMake (for building the project)

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
3. **Run the Game with Referee**: Use the `Referee` class to manage game flow, enforce rules, and display results.

### Example Code

Below is an example code snippet showing how to set up and run a game:

```cpp
#include "game.h"
#include "referee.h"
#include "random_agent.h"
#include "stratego.h"

int main() {
    // Create a game instance
    std::shared_ptr<Game> game = std::make_shared<StrategoGame>();

    // Create referee
    Referee referee;
    referee.SetGame(game);
    referee.AddAgent(0, std::make_unique<RandomAgent>());
    referee.AddAgent(1, std::make_unique<RandomAgent>());

    // Run the game
    referee.PlayGame();

    return 0;
}
```

### Customizing the Framework

To implement a new game or agent:
- Derive a new class from `Game` and implement the specific game rules.
- Extend the `Agent` class to define custom decision-making logic.

## Contributing

Contributions are welcome! Please fork the repository and create a pull request with your changes.

## License

This project is licensed under the MIT License. See `LICENSE` for more details.