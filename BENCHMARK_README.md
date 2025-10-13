# Benchmark Pipeline - Quick Start

This directory contains a complete benchmark pipeline for comparing ontime vs temporis solvers.

## Prerequisites
- Build ontime: `cd /home/pete/ontime && cargo build --release`
- Build temporis: `cd /home/pete/temporis && mkdir -p build && cd build && cmake .. && make`

## Usage

### 1. Generate benchmark games (150 games, 10-100 vertices)
```bash
python3 generate_games.py
```
Creates files in `benchmark/` directory with flat structure:
- test001.tg/dot/meta through test150.tg/dot/meta

### 2. Run benchmarks on both solvers
```bash
python3 run_benchmarks.py
```
Tests all 150 games and saves results to `benchmark/benchmark_results_detailed.json`

### 3. Create performance visualization
```bash
cd user_story_files && python3 create_clean_viz.py
```
Generates `clean_performance_comparison.png` showing performance comparison

## Files
- `generate_games.py` - Game generator with flat directory structure
- `run_benchmarks.py` - Benchmark runner for both solvers  
- `user_story_files/create_clean_viz.py` - Clean performance visualizer
