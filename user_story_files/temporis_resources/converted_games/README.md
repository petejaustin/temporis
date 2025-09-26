# Converted Temporal Graph Games

This directory contains 1000 temporal graph games converted from ontime format to temporis DOT format.

## Conversion Details

- **Source**: ontime .tg format with temporal conditions
- **Target**: temporis DOT format with Presburger arithmetic constraints
- **Node attributes**: name, player, target (randomly assigned)
- **Edge constraints**: Converted temporal conditions

## Format Changes

### Temporal Conditions
- `(= (mod t 3) 0)` → `time % 3 == 0`
- `(0,3,7,12)` → `time == 0 || time == 3 || time == 7 || time == 12`
- `(not (= (mod t 4) 0))` → `!(time % 4 == 0)`
- `(or (...) (...))` → `(...) || (...)`
- `(and (...) (...))` → `(...) && (...)`

### Node Attributes
- `owner[0]` → `player=0`
- `owner[1]` → `player=1`
- Target nodes assigned randomly or based on label hints

## Usage

Run any converted game with temporis:
```bash
./temporis converted_games/game_0001_chain_3.dot
./temporis converted_games/game_0201_branch_3_3.dot
```

## Game Types

- Chain graphs: `game_0001_*.dot` to `game_0200_*.dot`
- Branching graphs: `game_0201_*.dot` to `game_0400_*.dot`
- Cycle graphs: `game_0401_*.dot` to `game_0600_*.dot`
- Complex graphs: `game_0601_*.dot` to `game_1000_*.dot`
