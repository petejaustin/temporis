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
    """Generate benchmark games in temporal_games directory"""
    
    generator = BenchmarkGameGenerator()
    
    # Create temporal_games directory
    benchmark_dir = "/home/pete/temporis/temporal_games"
    os.makedirs(benchmark_dir, exist_ok=True)
    
    # Clean existing files
    for file in os.listdir(benchmark_dir):
        if file.startswith("test"):
            os.remove(os.path.join(benchmark_dir, file))
    
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
            tg_filename = f"{benchmark_dir}/test{game_id:03d}.tg"
            with open(tg_filename, 'w') as f:
                f.write(tg_content)
            
            # Write .dot file for temporis  
            dot_filename = f"{benchmark_dir}/test{game_id:03d}.dot"
            with open(dot_filename, 'w') as f:
                f.write(dot_content)
            
            # Write metadata
            meta_filename = f"{benchmark_dir}/test{game_id:03d}.meta"
            with open(meta_filename, 'w') as f:
                f.write(f"game_id: {game_id}\n")
                f.write(f"vertices: {size}\n") 
                f.write(f"time_bound: {time_bound}\n")
            
            game_id += 1
        
        print("✓")
    
    print("=" * 55)
    print(f"Generated {total_games} temporal benchmark games")
    print(f"Sizes: {sizes} (15 games each)")
    print(f"Files created in {benchmark_dir}/")
    print(f"  - test001.tg/dot/meta through test{total_games:03d}.tg/dot/meta")

if __name__ == "__main__":
    # Set random seed for reproducible benchmarks
    random.seed(42)
    generate_temporal_benchmark_games()
