#!/usr/bin/env python3
"""
Benchmark Runner
Runs performance tests on both ontime and temporis solvers
Generates detailed benchmark results for visualization
"""

import subprocess
import time
import json
import os
import glob
from typing import Dict, List, Tuple, Optional
import statistics

class BenchmarkRunner:
    def __init__(self, benchmark_dir="/home/pete/temporis/benchmark"):
        self.benchmark_dir = benchmark_dir
        self.ontime_path = "/home/pete/ontime/target/release/ontime"
        self.temporis_path = "/home/pete/temporis/build/temporis"
        
    def run_ontime_solver(self, tg_file: str, dot_file: str, time_bound: int, timeout: int = 10) -> Tuple[Optional[float], bool, str]:
        """Run ontime solver on a .tg file"""
        try:
            # Extract targets from dot file
            targets = self.extract_targets_from_dot(dot_file)
            if not targets:
                return None, False, "ERROR: No targets found in DOT file"
            
            target_set = ",".join(targets)
            
            start_time = time.time()
            result = subprocess.run(
                [self.ontime_path, tg_file, target_set, str(time_bound)],
                capture_output=True,
                text=True,
                timeout=timeout
            )
            end_time = time.time()
            
            runtime = end_time - start_time
            success = result.returncode == 0
            output = result.stdout if success else result.stderr
            
            return runtime, success, output
            
        except subprocess.TimeoutExpired:
            return None, False, "TIMEOUT"
        except Exception as e:
            return None, False, f"ERROR: {str(e)}"
    
    def run_temporis_solver(self, dot_file: str, timeout: int = 10) -> Tuple[Optional[float], bool, str]:
        """Run temporis solver on a .dot file"""
        try:
            start_time = time.time()
            result = subprocess.run(
                [self.temporis_path, dot_file],
                capture_output=True,
                text=True,
                timeout=timeout
            )
            end_time = time.time()
            
            runtime = end_time - start_time
            success = result.returncode == 0
            output = result.stdout if success else result.stderr
            
            return runtime, success, output
            
        except subprocess.TimeoutExpired:
            return None, False, "TIMEOUT"
        except Exception as e:
            return None, False, f"ERROR: {str(e)}"
    
    def extract_targets_from_dot(self, dot_file: str) -> List[str]:
        """Extract target nodes from DOT file"""
        targets = []
        try:
            with open(dot_file, 'r') as f:
                for line in f:
                    if 'target=1' in line:
                        # Extract node name from line like: v0 [name="v0", player=0, target=1];
                        parts = line.strip().split()
                        if parts:
                            node_name = parts[0]
                            targets.append(node_name)
        except Exception:
            pass
        return targets
    
    def get_game_metadata(self, game_id: int) -> Dict:
        """Read game metadata from .meta file"""
        meta_file = f"{self.benchmark_dir}/test{game_id:03d}.meta"
        metadata = {"game_id": game_id, "vertices": 0, "time_bound": 0}
        
        if os.path.exists(meta_file):
            with open(meta_file, 'r') as f:
                for line in f:
                    if ':' in line:
                        key, value = line.strip().split(': ', 1)
                        if key in ['vertices', 'time_bound']:
                            metadata[key] = int(value)
        
        return metadata
    
    def run_single_benchmark(self, game_id: int, verbose: bool = False) -> Dict:
        """Run benchmark for a single game"""
        tg_file = f"{self.benchmark_dir}/test{game_id:03d}.tg"
        dot_file = f"{self.benchmark_dir}/test{game_id:03d}.dot"
        
        if not os.path.exists(tg_file) or not os.path.exists(dot_file):
            return None
        
        metadata = self.get_game_metadata(game_id)
        
        # Run ontime
        ontime_runtime, ontime_success, ontime_output = self.run_ontime_solver(
            tg_file, dot_file, metadata["time_bound"])
        
        # Run temporis  
        temporis_runtime, temporis_success, temporis_output = self.run_temporis_solver(dot_file)
        
        result = {
            "game_id": game_id,
            "vertices": metadata["vertices"],
            "time_bound": metadata["time_bound"],
            "ontime": {
                "runtime": ontime_runtime,
                "success": ontime_success,
                "output": ontime_output[:200] if ontime_output else ""
            },
            "temporis": {
                "runtime": temporis_runtime,
                "success": temporis_success,
                "output": temporis_output[:200] if temporis_output else ""
            }
        }
        
        if verbose:
            status = ""
            if ontime_success and temporis_success:
                speedup = ontime_runtime / temporis_runtime if temporis_runtime > 0 else float('inf')
                status = f"✓ Both solved - temporis {speedup:.1f}x faster"
            elif ontime_success:
                status = "⚠ Only ontime solved"
            elif temporis_success:
                status = "⚠ Only temporis solved" 
            else:
                status = "✗ Both failed"
            
            print(f"  Game {game_id:3d} ({metadata['vertices']:3d}v): {status}")
        
        return result
    
    def run_all_benchmarks(self, verbose: bool = True) -> Dict:
        """Run benchmarks on all games"""
        
        if verbose:
            print("Running comprehensive benchmarks...")
            print("=" * 60)
        
        # Find all games
        tg_files = sorted(glob.glob(f"{self.benchmark_dir}/test*.tg"))
        game_ids = [int(f.split('test')[1].split('.')[0]) for f in tg_files]
        
        if not game_ids:
            print(f"No benchmark games found in {self.benchmark_dir}")
            return {}
        
        results = []
        sizes = list(range(10, 101, 10))
        
        for size in sizes:
            if verbose:
                print(f"\nTesting {size:3d} vertex games:")
            
            size_results = []
            size_start = (size // 10 - 1) * 15 + 1
            size_end = size_start + 15
            
            for game_id in range(size_start, size_end):
                if game_id in game_ids:
                    result = self.run_single_benchmark(game_id, verbose)
                    if result:
                        results.append(result)
                        size_results.append(result)
            
            if size_results and verbose:
                successful_pairs = [r for r in size_results 
                                  if r["ontime"]["success"] and r["temporis"]["success"]]
                if successful_pairs:
                    ontime_times = [r["ontime"]["runtime"] for r in successful_pairs]
                    temporis_times = [r["temporis"]["runtime"] for r in successful_pairs]
                    avg_ontime = statistics.mean(ontime_times)
                    avg_temporis = statistics.mean(temporis_times)
                    avg_speedup = avg_ontime / avg_temporis if avg_temporis > 0 else float('inf')
                    
                    print(f"    Average: ontime={avg_ontime:.3f}s, temporis={avg_temporis:.3f}s")
                    print(f"    Speedup: {avg_speedup:.1f}x faster")
        
        # Generate summary statistics
        summary = self._generate_summary(results)
        
        benchmark_data = {
            "timestamp": time.time(),
            "total_games": len(results),
            "results": results,
            "summary": summary
        }
        
        # Save detailed results
        output_file = f"{self.benchmark_dir}/benchmark_results_detailed.json"
        with open(output_file, 'w') as f:
            json.dump(benchmark_data, f, indent=2)
        
        if verbose:
            print("=" * 60)
            print(f"Benchmark completed!")
            print(f"Results saved to: {output_file}")
            print(f"Total games tested: {len(results)}")
            print(f"Success rate: {summary['overall_success_rate']:.1%}")
            print(f"Average speedup: {summary['average_speedup']:.1f}x")
        
        return benchmark_data
    
    def _generate_summary(self, results: List[Dict]) -> Dict:
        """Generate summary statistics"""
        if not results:
            return {}
        
        successful_pairs = [r for r in results 
                          if r["ontime"]["success"] and r["temporis"]["success"]]
        
        ontime_successes = len([r for r in results if r["ontime"]["success"]])
        temporis_successes = len([r for r in results if r["temporis"]["success"]])
        
        summary = {
            "total_games": len(results),
            "ontime_successes": ontime_successes,
            "temporis_successes": temporis_successes,
            "both_successful": len(successful_pairs),
            "overall_success_rate": len(successful_pairs) / len(results),
            "ontime_success_rate": ontime_successes / len(results),
            "temporis_success_rate": temporis_successes / len(results)
        }
        
        if successful_pairs:
            ontime_times = [r["ontime"]["runtime"] for r in successful_pairs]
            temporis_times = [r["temporis"]["runtime"] for r in successful_pairs]
            speedups = [o/t for o, t in zip(ontime_times, temporis_times) if t > 0]
            
            summary.update({
                "average_speedup": statistics.mean(speedups) if speedups else 0,
                "median_speedup": statistics.median(speedups) if speedups else 0,
                "min_speedup": min(speedups) if speedups else 0,
                "max_speedup": max(speedups) if speedups else 0,
                "avg_ontime_runtime": statistics.mean(ontime_times),
                "avg_temporis_runtime": statistics.mean(temporis_times)
            })
        
        return summary

def main():
    """Main benchmark execution"""
    
    print("Starting benchmark runner...")
    
    # Check if benchmark games exist
    benchmark_dir = "/home/pete/temporis/benchmark"
    if not os.path.exists(benchmark_dir):
        print(f"Benchmark directory not found: {benchmark_dir}")
        print("Run generate_games.py first to create benchmark games")
        return
    
    print(f"Benchmark directory found: {benchmark_dir}")
    
    runner = BenchmarkRunner(benchmark_dir)
    
    # Check solver availability
    if not os.path.exists(runner.ontime_path):
        print(f"Ontime solver not found: {runner.ontime_path}")
        return
    
    if not os.path.exists(runner.temporis_path):
        print(f"Temporis solver not found: {runner.temporis_path}")
        return
    
    # Run benchmarks
    results = runner.run_all_benchmarks(verbose=True)
    
    if results:
        print(f"\nBenchmark data ready for visualization!")
        print(f"Use user_story_files/create_clean_viz.py to create charts")

if __name__ == "__main__":
    main()
