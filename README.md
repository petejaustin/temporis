# Temporis: Presburger Temporal Game E## Usage

### Compilation
```bash
cd temporis
g++ -std=c++20 -I../ggg/include main.cpp -o main
```

### Execution
```bash
# DOT file input is required
./main example_temporal.dot

# Use complex example
./main complex_temporal_game.dot

# Usage message shown if no file provided
./main
```rectory contains examples of 2-player temporal games using Presburger arithmetic constraints for edge availability, built with the Game Graph Gym (GGG) library.

## Files Overview

### `main.cpp`
**2-player temporal game with Presburger arithmetic constraints**
- Simple vertex naming (v0, v1, v2, ...) following GGG conventions
- 2-player game structure (player 0 and player 1 vertices)
- Edge availability determined solely by Presburger arithmetic formulas
- DOT file input parser for game specification (no hardcoded examples)

**Key Features:**
- PresburgerTemporalGameManager class
- Complete DOT file parser with regex-based parsing
- Presburger arithmetic constraint evaluation
- Temporal game state simulation
- Real-time edge availability analysis based on mathematical constraints

### `example_temporal.dot`
**DOT format input file for Presburger temporal games**
- Simple vertex declarations with player assignments: `v0 [name="v0", player=0];`
- Edge constraints using Presburger arithmetic: `v0 -> v1 [label="e0", constraint="t >= 2"];`
- Follows GGG test suite file format conventions

### `complex_temporal_game.dot`
**More complex example with additional constraint patterns**
- Demonstrates various Presburger constraint types
- Larger game graph with 6 vertices and 8 edges
- Mixed constraint patterns showing different temporal behaviors

**Constraint Examples:**
- `t >= 2` - Edge active from time 2 onwards
- `t = 3` - Edge active only at time 3
- `t <= 5` - Edge active up to time 5
- No constraint - Edge always active

## Usage

### Compilation
```bash
cd temporis
g++ -std=c++20 -I../ggg/include presburger_temporal_graph_example.cpp -o presburger_temporal_graph_example
```

### Execution
```bash
# Use default example_temporal.dot
./presburger_temporal_graph_example

# Use custom DOT file
./presburger_temporal_graph_example complex_temporal_game.dot
```

## Game Structure

The implementation creates a 2-player temporal game where:
- **Player 0** owns vertices: v0, v2, v4
- **Player 1** owns vertices: v1, v3
- **Edge availability** is determined by Presburger arithmetic constraints evaluated at each time step

## Presburger Constraints

The system supports basic Presburger arithmetic including:
- **Equality**: `t = 3` (edge active only at time 3)
- **Inequalities**: `t >= 2`, `t <= 5` (edge active when condition holds)
- **Existential quantification**: Can be extended with `∃k. (constraint involving k)`

## Example Output

```
=== Game Structure ===
Player 0 vertices: v0 v2 v4 
Player 1 vertices: v1 v3 

=== Presburger Temporal Game State at Time 3 ===
Edge Availability (Presburger Constraints):
  v0 -> v1 (e0): ACTIVE [t >= 2]
  v0 -> v4 (e4): ACTIVE
  v1 -> v2 (e1): ACTIVE [t = 3]
  v2 -> v3 (e2): ACTIVE [t <= 5]
  v3 -> v4 (e3): INACTIVE [t >= 4]
```

This demonstrates how mathematical constraints control the temporal availability of game edges, creating dynamic game graphs that evolve over time according to precise Presburger arithmetic formulas.

## Technical Architecture

### Graph Field Definitions
The example uses simplified GGG macro system for 2-player games:
```cpp
#define PRESBURGER_TEMPORAL_VERTEX_FIELDS(X) \
    X(std::string, name)                     \
    X(int, player)

#define PRESBURGER_TEMPORAL_EDGE_FIELDS(X) \
    X(std::string, label)

DEFINE_GAME_GRAPH(PresburgerTemporal, PRESBURGER_TEMPORAL_VERTEX_FIELDS, PRESBURGER_TEMPORAL_EDGE_FIELDS, PRESBURGER_TEMPORAL_GRAPH_FIELDS)
```

### Presburger Arithmetic
Mathematical constraint evaluation through:
- `PresburgerTerm` - Variables and constants in linear arithmetic
- `PresburgerFormula` - Equality, inequality, and logical operations
- Real-time constraint evaluation at each time step

### DOT File Format
Input-only format compatible with GGG test suite structure:
```dot
digraph temporal_game {
    v0 [name="v0", player=0];
    v1 [name="v1", player=1];
    v0 -> v1 [label="e0", constraint="t >= 2"];
}
```

**Parsing Features:**
- Complete regex-based DOT file parsing
- Automatic vertex and edge detection
- Constraint parsing for `t >= n`, `t = n`, `t <= n` patterns
- No hardcoded examples - all content from input files

## Design Philosophy

This implementation focuses on:
- **Input-Only Operation**: No hardcoded examples, all content from DOT files
- **Complete DOT Parsing**: Regex-based parser for vertices, edges, and constraints
- **Simplicity**: Minimal vertex properties, only name and player
- **Mathematical Precision**: Edge availability controlled solely by Presburger constraints
- **GGG Compatibility**: Follows Game Graph Gym naming and structural conventions
- **2-Player Games**: Standard game-theoretic framework
- **Required Input Files**: Enforces proper usage by requiring DOT file arguments
