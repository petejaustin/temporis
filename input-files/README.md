# Temporis Test Input Files

This directory contains a curated set of test input files for the Temporis temporal reachability game system. All files follow the current syntax with vertex target attributes.

## Test Files Overview

### 1. **basic_reachability.dot**
- **Purpose**: Basic reachability game without temporal constraints
- **Features**: Simple 4-vertex game, single target
- **Target**: v2
- **Use Case**: Testing fundamental reachability solving

### 2. **simple_temporal.dot**
- **Purpose**: Introduction to temporal constraints
- **Features**: Linear time constraints (>=, <=), single target
- **Target**: v2
- **Constraints**: Time windows and delays
- **Use Case**: Basic temporal constraint testing

### 3. **multiple_targets.dot**
- **Purpose**: Multiple target vertex demonstration
- **Features**: 3 target vertices with different temporal windows
- **Targets**: v2, v4, v6
- **Use Case**: Testing multiple target handling

### 4. **existential_demo.dot**
- **Purpose**: Existential quantifier constraints
- **Features**: Even/odd time patterns using exists quantifiers
- **Target**: v3
- **Constraints**: Mathematical time patterns
- **Use Case**: Advanced temporal logic testing

### 5. **complex_temporal.dot**
- **Purpose**: Advanced temporal relationships
- **Features**: Multiple quantifiers, complex patterns, time windows
- **Target**: v4
- **Constraints**: Multiples of numbers, combined conditions
- **Use Case**: Stress testing constraint parsing

### 6. **legacy_game.dot**
- **Purpose**: Game without reachability objectives
- **Features**: No target vertices, pure temporal analysis
- **Targets**: None
- **Use Case**: Legacy compatibility, pure temporal analysis

### 7. **performance_test.dot**
- **Purpose**: Large game for performance testing
- **Features**: 11 vertices, complex temporal relationships
- **Target**: v10
- **Use Case**: Solver performance evaluation

## Syntax Format

All reachability games use the vertex target attribute syntax:

```dot
v0 [name="v0", player=0];           // Regular vertex
v1 [name="v1", player=0, target=1]; // Target vertex
```

## Testing Commands

```bash
# Basic test
./temporis input-files/basic_reachability.dot

# Verbose output
./temporis -v input-files/simple_temporal.dot

# Legacy test (no targets)
./temporis input-files/legacy_game.dot
```

## Expected Behavior

- **Reachability games**: Show target vertices and winning regions
- **Legacy games**: Show only basic game statistics
- **Verbose mode**: Include game structure and temporal edge analysis
- **Default mode**: Clean, minimal output
