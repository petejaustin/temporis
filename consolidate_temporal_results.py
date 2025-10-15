#!/usr/bin/env python3
"""
Consolidate temporal solver benchmark results from separate JSON files.
Combines results from temporis and ontime benchmarks into a unified analysis.
"""

import json
import sys
import statistics
from pathlib import Path

def load_results(json_file):
    """Load benchmark results from JSON file."""
    try:
        with open(json_file, 'r') as f:
            data = json.load(f)
            # Extract results array from the benchmark JSON structure
            if isinstance(data, dict) and 'results' in data:
                return data['results']
            elif isinstance(data, list):
                return data
            else:
                print(f"Error: Unexpected JSON structure in {json_file}")
                return None
    except FileNotFoundError:
        print(f"Error: Could not find {json_file}")
        return None
    except json.JSONDecodeError:
        print(f"Error: Invalid JSON in {json_file}")
        return None

def analyze_results(results, solver_name):
    """Analyze benchmark results for a single solver."""
    if not results:
        return None
    
    # Extract timing data
    times = [result['time'] for result in results if result['status'] == 'success']
    vertices = [result['vertices'] for result in results if result['status'] == 'success']
    
    if not times:
        return {
            'solver': solver_name,
            'total_games': len(results),
            'successful': 0,
            'failed': len(results),
            'success_rate': 0.0
        }
    
    # Calculate statistics
    analysis = {
        'solver': solver_name,
        'total_games': len(results),
        'successful': len(times),
        'failed': len(results) - len(times),
        'success_rate': len(times) / len(results) * 100,
        'times': {
            'mean': statistics.mean(times),
            'median': statistics.median(times),
            'min': min(times),
            'max': max(times),
            'std_dev': statistics.stdev(times) if len(times) > 1 else 0.0
        },
        'vertex_counts': {
            'min': min(vertices) if vertices else 0,
            'max': max(vertices) if vertices else 0,
            'unique': sorted(set(vertices)) if vertices else []
        }
    }
    
    # Calculate performance by vertex count
    vertex_performance = {}
    for result in results:
        if result['status'] == 'success':
            v_count = result['vertices']
            if v_count not in vertex_performance:
                vertex_performance[v_count] = []
            vertex_performance[v_count].append(result['time'])
    
    # Summarize by vertex count
    vertex_summary = {}
    for v_count, v_times in vertex_performance.items():
        vertex_summary[v_count] = {
            'count': len(v_times),
            'mean_time': statistics.mean(v_times),
            'median_time': statistics.median(v_times),
            'min_time': min(v_times),
            'max_time': max(v_times)
        }
    
    analysis['performance_by_vertices'] = vertex_summary
    return analysis

def compare_solvers(temporis_analysis, ontime_analysis):
    """Compare performance between temporis and ontime."""
    if not temporis_analysis or not ontime_analysis:
        return None
    
    comparison = {
        'temporis_vs_ontime': {
            'speed_ratio': ontime_analysis['times']['mean'] / temporis_analysis['times']['mean'],
            'temporis_faster_by': f"{ontime_analysis['times']['mean'] / temporis_analysis['times']['mean']:.2f}x",
            'median_ratio': ontime_analysis['times']['median'] / temporis_analysis['times']['median']
        },
        'vertex_comparison': {}
    }
    
    # Compare by vertex count
    for v_count in temporis_analysis['performance_by_vertices']:
        if v_count in ontime_analysis['performance_by_vertices']:
            t_time = temporis_analysis['performance_by_vertices'][v_count]['mean_time']
            o_time = ontime_analysis['performance_by_vertices'][v_count]['mean_time']
            comparison['vertex_comparison'][v_count] = {
                'temporis_mean': t_time,
                'ontime_mean': o_time,
                'speed_ratio': o_time / t_time,
                'faster_solver': 'temporis' if t_time < o_time else 'ontime'
            }
    
    return comparison

def main():
    """Main consolidation function."""
    if len(sys.argv) != 4:
        print("Usage: python3 consolidate_temporal_results.py <temporis_json> <ontime_json> <output_json>")
        print("\nExample:")
        print("  python3 consolidate_temporal_results.py temporis_results/consolidated_results.json ontime_results/consolidated_results.json combined_analysis.json")
        sys.exit(1)
    
    temporis_file = sys.argv[1]
    ontime_file = sys.argv[2]
    output_file = sys.argv[3]
    
    print("Temporal Solver Benchmark Consolidation")
    print("=" * 45)
    
    # Load results
    print(f"Loading temporis results from: {temporis_file}")
    temporis_results = load_results(temporis_file)
    
    print(f"Loading ontime results from: {ontime_file}")
    ontime_results = load_results(ontime_file)
    
    if not temporis_results or not ontime_results:
        sys.exit(1)
    
    # Analyze results
    print("\nAnalyzing temporis performance...")
    temporis_analysis = analyze_results(temporis_results, "Temporis")
    
    print("Analyzing ontime performance...")
    ontime_analysis = analyze_results(ontime_results, "Ontime")
    
    print("Comparing solver performance...")
    comparison = compare_solvers(temporis_analysis, ontime_analysis)
    
    # Create consolidated report in GGG-compatible format
    # Combine both solver results into a single results array
    all_results = []
    
    # Add temporis results
    for result in temporis_results:
        all_results.append(result)
    
    # Add ontime results  
    for result in ontime_results:
        all_results.append(result)
    
    # Create GGG benchmark.py compatible JSON structure
    consolidated = {
        "benchmark_info": {
            "timestamp": __import__('time').time(),
            "games_dir": "combined_temporal_analysis",
            "total_tests": len(all_results),
            "solvers_tested": 2,
            "games_tested": len(temporis_results),  # Unique games
            "timeout_seconds": 300,
            "consolidation_metadata": {
                "temporis_source": temporis_file,
                "ontime_source": ontime_file,
                "analysis_timestamp": __import__('time').time()
            }
        },
        "results": all_results
    }
    
    # Save consolidated results
    with open(output_file, 'w') as f:
        json.dump(consolidated, f, indent=2)
    
    # Print summary
    print(f"\n" + "=" * 45)
    print("CONSOLIDATED ANALYSIS SUMMARY")
    print("=" * 45)
    print(f"Total results combined: {len(all_results)}")
    print(f"Unique games: {len(temporis_results)}")
    print(f"")
    print(f"Temporis Performance:")
    print(f"  Success rate: {temporis_analysis['success_rate']:.1f}%")
    print(f"  Mean time: {temporis_analysis['times']['mean']:.6f}s")
    print(f"  Median time: {temporis_analysis['times']['median']:.6f}s")
    print(f"")
    print(f"Ontime Performance:")
    print(f"  Success rate: {ontime_analysis['success_rate']:.1f}%")
    print(f"  Mean time: {ontime_analysis['times']['mean']:.6f}s")
    print(f"  Median time: {ontime_analysis['times']['median']:.6f}s")
    print(f"")
    fastest_solver = 'temporis' if temporis_analysis['times']['mean'] < ontime_analysis['times']['mean'] else 'ontime'
    speed_difference = max(temporis_analysis['times']['mean'], ontime_analysis['times']['mean']) / min(temporis_analysis['times']['mean'], ontime_analysis['times']['mean'])
    print(f"Overall Comparison:")
    print(f"  Fastest solver: {fastest_solver}")
    print(f"  Speed difference: {speed_difference:.2f}x")
    print(f"")
    print(f"GGG-compatible results saved to: {output_file}")
    print(f"  Format: Compatible with /ggg/extra/scripts/plot_time_by_vertex_count.py")

if __name__ == "__main__":
    main()
