#!/usr/bin/env python3
"""
Performance Visualizer for Ontime vs Temporis Benchmark Results
Creates line plots showing mean execution times across different game sizes
"""

import subprocess
import time
import os
import glob
import json
import statistics
import matplotlib.pyplot as plt
import numpy as np
from typing import Dict, List, Tuple

def run_ontime_benchmark(tg_file: str, time_bound: int) -> Tuple[bool, float, str]:
    """Run ontime solver and return timing"""
    try:
        # Extract target from .tg file (use first target found)
        targets = []
        with open(tg_file, 'r') as f:
            content = f.read()
            # Simple extraction - look for nodes and pick one as target
            lines = content.split('\n')
            for line in lines:
                if line.strip().startswith('node ') and ':' in line:
                    node_name = line.split()[1].rstrip(':')
                    targets.append(node_name)
                    break  # Just use first node as target for timing purposes
        
        if not targets:
            return False, 0.0, "No targets found"
        
        cmd = ["/home/pete/ontime/target/debug/ontime", tg_file, targets[0], str(time_bound)]
        
        start_time = time.time()
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        elapsed_time = time.time() - start_time
        
        if result.returncode != 0:
            return False, elapsed_time, result.stderr.strip()
        
        return True, elapsed_time, ""
        
    except subprocess.TimeoutExpired:
        return False, 60.0, "Timeout"
    except Exception as e:
        return False, 0.0, str(e)

def run_temporis_benchmark(dot_file: str, time_bound: int) -> Tuple[bool, float, str]:
    """Run temporis solver and return timing"""
    try:
        cmd = ["/home/pete/temporis/temporis", "-t", str(time_bound), dot_file]
        
        start_time = time.time()
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        elapsed_time = time.time() - start_time
        
        if result.returncode != 0:
            return False, elapsed_time, result.stderr.strip()
        
        return True, elapsed_time, ""
        
    except subprocess.TimeoutExpired:
        return False, 60.0, "Timeout"
    except Exception as e:
        return False, 0.0, str(e)

def read_metadata(meta_file: str) -> Dict:
    """Read game metadata"""
    metadata = {}
    try:
        with open(meta_file, 'r') as f:
            for line in f:
                if ':' in line:
                    key, value = line.strip().split(':', 1)
                    metadata[key.strip()] = value.strip()
        return metadata
    except:
        return {}

def collect_benchmark_data():
    """Collect benchmark data for visualization"""
    
    sizes = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
    results_by_size = {}
    
    print("Collecting benchmark data...")
    print("=" * 50)
    
    for size in sizes:
        print(f"Processing {size:3d} vertex games...", end=" ", flush=True)
        
        size_dir = f"/home/pete/temporis/benchmark/{size:03d}_vertices"
        meta_files = sorted(glob.glob(f"{size_dir}/test*.meta"))
        
        ontime_times = []
        temporis_times = []
        successful_games = 0
        
        for meta_file in meta_files:
            metadata = read_metadata(meta_file)
            time_bound = int(metadata.get('time_bound', '10'))
            
            base_name = meta_file.replace('.meta', '')
            tg_file = f"{base_name}.tg"
            dot_file = f"{base_name}.dot"
            
            # Run ontime
            ontime_success, ontime_time, _ = run_ontime_benchmark(tg_file, time_bound)
            
            # Run temporis
            temporis_success, temporis_time, _ = run_temporis_benchmark(dot_file, time_bound)
            
            if ontime_success and temporis_success:
                ontime_times.append(ontime_time)
                temporis_times.append(temporis_time)
                successful_games += 1
        
        if ontime_times and temporis_times:
            results_by_size[size] = {
                'ontime_mean': statistics.mean(ontime_times),
                'ontime_std': statistics.stdev(ontime_times) if len(ontime_times) > 1 else 0,
                'temporis_mean': statistics.mean(temporis_times),
                'temporis_std': statistics.stdev(temporis_times) if len(temporis_times) > 1 else 0,
                'count': successful_games
            }
        
        print(f"✓ ({successful_games}/15 successful)")
    
    return results_by_size

def create_visualization(results_by_size):
    """Create performance visualization"""
    
    # Extract data for plotting
    sizes = sorted(results_by_size.keys())
    ontime_means = [results_by_size[size]['ontime_mean'] for size in sizes]
    ontime_stds = [results_by_size[size]['ontime_std'] for size in sizes]
    temporis_means = [results_by_size[size]['temporis_mean'] for size in sizes]
    temporis_stds = [results_by_size[size]['temporis_std'] for size in sizes]
    
    # Create the main plot
    plt.figure(figsize=(12, 8))
    
    # Plot 1: Execution times with error bars
    plt.subplot(2, 2, 1)
    plt.errorbar(sizes, ontime_means, yerr=ontime_stds, marker='o', linewidth=2, 
                capsize=3, label='Ontime', color='#e74c3c')
    plt.errorbar(sizes, temporis_means, yerr=temporis_stds, marker='s', linewidth=2,
                capsize=3, label='Temporis', color='#3498db')
    plt.xlabel('Number of Vertices')
    plt.ylabel('Execution Time (seconds)')
    plt.title('Mean Execution Time by Graph Size')
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.yscale('log')
    
    # Plot 2: Speedup ratio
    plt.subplot(2, 2, 2)
    speedup_ratios = [ontime_means[i] / temporis_means[i] for i in range(len(sizes))]
    plt.plot(sizes, speedup_ratios, marker='D', linewidth=2, color='#2ecc71')
    plt.xlabel('Number of Vertices')
    plt.ylabel('Speedup Ratio (Ontime/Temporis)')
    plt.title('Temporis Speedup vs Ontime')
    plt.grid(True, alpha=0.3)
    
    # Plot 3: Linear scale comparison
    plt.subplot(2, 2, 3)
    plt.plot(sizes, ontime_means, marker='o', linewidth=2, label='Ontime', color='#e74c3c')
    plt.plot(sizes, temporis_means, marker='s', linewidth=2, label='Temporis', color='#3498db')
    plt.xlabel('Number of Vertices')
    plt.ylabel('Execution Time (seconds)')
    plt.title('Execution Time (Linear Scale)')
    plt.legend()
    plt.grid(True, alpha=0.3)
    
    # Plot 4: Performance scaling analysis
    plt.subplot(2, 2, 4)
    # Normalize to smallest size for scaling analysis
    ontime_normalized = [t / ontime_means[0] for t in ontime_means]
    temporis_normalized = [t / temporis_means[0] for t in temporis_means]
    size_normalized = [s / sizes[0] for s in sizes]
    
    plt.loglog(size_normalized, ontime_normalized, marker='o', linewidth=2, 
              label='Ontime', color='#e74c3c')
    plt.loglog(size_normalized, temporis_normalized, marker='s', linewidth=2,
              label='Temporis', color='#3498db')
    
    # Add reference lines for different complexity classes
    x_ref = np.array(size_normalized)
    plt.loglog(x_ref, x_ref, '--', alpha=0.5, label='O(n)', color='gray')
    plt.loglog(x_ref, x_ref**2, '--', alpha=0.5, label='O(n²)', color='gray')
    plt.loglog(x_ref, x_ref**3, '--', alpha=0.5, label='O(n³)', color='gray')
    
    plt.xlabel('Relative Graph Size')
    plt.ylabel('Relative Execution Time')
    plt.title('Algorithmic Scaling Analysis')
    plt.legend()
    plt.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    # Save the plot
    plt.savefig('/home/pete/temporis/performance_comparison.png', dpi=300, bbox_inches='tight')
    plt.savefig('/home/pete/temporis/performance_comparison.pdf', bbox_inches='tight')
    
    # Show summary statistics
    print("\n" + "=" * 60)
    print("PERFORMANCE VISUALIZATION SUMMARY")
    print("=" * 60)
    print(f"{'Size':<6} {'Count':<6} {'Ontime':<12} {'Temporis':<12} {'Speedup':<8}")
    print("-" * 60)
    
    for size in sizes:
        data = results_by_size[size]
        speedup = data['ontime_mean'] / data['temporis_mean']
        print(f"{size:<6} {data['count']:<6} {data['ontime_mean']:<12.4f} "
              f"{data['temporis_mean']:<12.4f} {speedup:<8.1f}x")
    
    overall_speedup = sum(ontime_means) / sum(temporis_means)
    print("-" * 60)
    print(f"Overall speedup: {overall_speedup:.1f}x")
    print(f"Max speedup: {max(speedup_ratios):.1f}x at {sizes[speedup_ratios.index(max(speedup_ratios))]} vertices")
    print(f"Min speedup: {min(speedup_ratios):.1f}x at {sizes[speedup_ratios.index(min(speedup_ratios))]} vertices")
    
    print(f"\nVisualization saved to:")
    print(f"  - performance_comparison.png")
    print(f"  - performance_comparison.pdf")
    
    return results_by_size

def main():
    """Main function to run benchmark and create visualization"""
    
    print("Temporal Game Solver Performance Benchmark & Visualization")
    print("=" * 60)
    
    # Collect benchmark data
    results = collect_benchmark_data()
    
    if not results:
        print("No benchmark data collected. Please check your games.")
        return
    
    # Create visualization
    create_visualization(results)
    
    # Save detailed results
    with open("/home/pete/temporis/benchmark_results_detailed.json", 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\nDetailed results saved to: benchmark_results_detailed.json")

if __name__ == "__main__":
    main()
