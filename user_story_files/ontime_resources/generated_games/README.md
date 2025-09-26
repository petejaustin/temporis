# Generated Temporal Graph Games

This directory contains 1000 generated temporal graph games for the ontime solver.

## Game Types

- **Chain graphs (games 1-200)**: Simple linear chains with temporal conditions
- **Branching graphs (games 201-400)**: Tree-like structures with multiple paths
- **Cycle graphs (games 401-600)**: Circular structures with temporal loops
- **Complex graphs (games 601-1000)**: Dense graphs with varied connectivity

## Usage

Run any game with the ontime solver:
```bash
cargo run -- generated_games/game_0001_chain_5.tg "s0,s4" 100
```

## Temporal Conditions

The games use various temporal conditions:
- Modular arithmetic: `(= (mod t 3) 0)`
- Explicit time lists: `(0,3,7,12)`
- Boolean combinations: `(or (= (mod t 2) 0) (= (mod t 3) 1))`
- Negations: `(not (= (mod t 4) 0))`
