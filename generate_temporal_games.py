#!/usr/bin/env python3
"""
Generate temporal games for benchmarking
Creates games in the temporal_games directory
"""

import os
import random
import sys

# Add the current directory to path so we can import the generator
sys.path.append('/home/pete/temporis')
from generate_games import BenchmarkGameGenerator

def generate_temporal_benchmark_games():
    """Generate benchmark games in separate directories for each solver"""
    
    generator = BenchmarkGameGenerator()
    
    # Create separate directories for each solver
    temporis_dir = "/home/pete/temporis/temporis_games"
    ontime_dir = "/home/pete/temporis/ontime_games"
    
    os.makedirs(temporis_dir, exist_ok=True)
    os.makedirs(ontime_dir, exist_ok=True)
    
    # Clean existing files
    for directory in [temporis_dir, ontime_dir]:
        for file in os.listdir(directory):
            if file.startswith("test"):
                os.remove(os.path.join(directory, file))
    
    # Generate 150 games total: 15 games each for vertex counts 10, 20, 30, ..., 100
    sizes = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
    games_per_size = 15
    
    print("Generating 150 temporal benchmark games...")
    print("=" * 55)
    
    game_id = 1
    total_games = len(sizes) * games_per_size
    
    for size in sizes:
        print(f"Generating {size:2d} vertex games... ", end="", flush=True)
        
        for i in range(games_per_size):
            tg_content, dot_content, time_bound = generator.generate_benchmark_game(size, game_id)
            
            # Write .tg file for ontime
            tg_filename = f"{ontime_dir}/test{game_id:03d}.tg"
            with open(tg_filename, 'w') as f:
                f.write(tg_content)
            
            # Write .dot file for temporis  
            dot_filename = f"{temporis_dir}/test{game_id:03d}.dot"
            with open(dot_filename, 'w') as f:
                f.write(dot_content)
            
            game_id += 1
        
        print("✓")
    
    print("=" * 55)
    print(f"Generated {total_games} temporal benchmark games")
    print(f"Sizes: {sizes} (15 games each)")
    print(f"Temporis games (.dot): {temporis_dir}/")
    print(f"Ontime games (.tg):   {ontime_dir}/")
    print(f"  - test001 through test{total_games:03d}")

if __name__ == "__main__":
    # Set random seed for reproducible benchmarks
    random.seed(42)
    generate_temporal_benchmark_games()
