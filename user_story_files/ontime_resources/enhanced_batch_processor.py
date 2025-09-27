#!/usr/bin/env python3
"""
Enhanced batch processing script for ontime solver with detailed CSV output.
Collects game name, winning regions, and solve time for all games.
"""

import os
import subprocess
import argparse
import time
import re
from pathlib import Path
import json
import csv

def read_target_vertices(target_file):
    """Read target vertices from file, return list of target sets."""
    targets = []
    with open(target_file, 'r') as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            if line:
                target_set = [v.strip() for v in line.split(',')]
                targets.append(target_set)
            else:
                targets.append([])
    return targets

def parse_ontime_output(stdout):
    """Parse ontime output to extract winning regions."""
    winning_regions = {}
    
    # Parse lines like: W_10000 = {"s0", "s1"}
    pattern = r'W_(\d+)\s*=\s*\{([^}]*)\}'
    matches = re.findall(pattern, stdout)
    
    for time_str, vertices_str in matches:
        time_val = int(time_str)
        if vertices_str.strip():
            # Parse vertices: "s0", "s1" -> [s0, s1]
            vertices = []
            vertex_matches = re.findall(r'"([^"]+)"', vertices_str)
            vertices = vertex_matches
        else:
            vertices = []
        
        winning_regions[time_val] = vertices
    
    return winning_regions

def run_ontime_command(game_file, targets, time_bound, timeout=120):
    """Run a single ontime command and return parsed results."""
    if not targets:
        return {
            'success': False,
            'error': 'No target vertices for this game',
            'winning_regions': {},
            'runtime': 0
        }
    
    target_str = ','.join(targets)
    cmd = ['cargo', 'run', '--release', '--', str(game_file), target_str, str(time_bound)]
    
    start_time = time.time()
    try:
        result = subprocess.run(
            cmd,
            cwd='/home/pete/ontime',
            capture_output=True,
            text=True,
            timeout=timeout
        )
        runtime = time.time() - start_time
        
        if result.returncode == 0:
            winning_regions = parse_ontime_output(result.stdout)
        else:
            winning_regions = {}
        
        return {
            'success': result.returncode == 0,
            'stdout': result.stdout,
            'stderr': result.stderr,
            'returncode': result.returncode,
            'runtime': runtime,
            'winning_regions': winning_regions,
            'command': ' '.join(cmd)
        }
    except subprocess.TimeoutExpired:
        return {
            'success': False,
            'error': f'Command timed out after {timeout} seconds',
            'winning_regions': {},
            'runtime': timeout,
            'command': ' '.join(cmd)
        }
    except Exception as e:
        return {
            'success': False,
            'error': str(e),
            'winning_regions': {},
            'runtime': time.time() - start_time,
            'command': ' '.join(cmd)
        }

def save_detailed_csv(results, output_file, time_bound):
    """Save results to a detailed CSV file."""
    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        
        # Header
        writer.writerow([
            'game_number',
            'game_name', 
            'game_file',
            'target_vertices',
            'success',
            'solve_time_seconds',
            f'winning_region_t0',
            f'winning_region_t{time_bound}',
            'error_message'
        ])
        
        # Data rows
        for r in results:
            game_name = Path(r['game_file']).stem if 'game_file' in r else ''
            target_str = ','.join(r.get('targets', []))
            
            winning_regions = r.get('winning_regions', {})
            w0 = ','.join(winning_regions.get(0, []))
            wT = ','.join(winning_regions.get(time_bound, []))
            
            error_msg = r.get('error', '')
            if not error_msg and not r.get('success', False):
                error_msg = 'Failed - see stderr'
            
            writer.writerow([
                r.get('game_number', 0),
                game_name,
                r.get('game_file', ''),
                target_str,
                r.get('success', False),
                f"{r.get('runtime', 0):.3f}",
                w0,
                wT,
                error_msg
            ])

def process_all_games(games_dir, targets, time_bound=10000, game_numbers=None, 
                     output_dir=None, timeout=120, start_from=1):
    """Process all games and collect results."""
    
    results = []
    games_dir = Path(games_dir)
    
    # Get list of game files
    game_files = sorted(games_dir.glob('game_*.tg'))
    
    if not game_files:
        print(f"No game files found in {games_dir}")
        return []
    
    # Filter to specific game numbers if requested
    if game_numbers:
        filtered_files = []
        filtered_indices = []
        for game_num in game_numbers:
            matches = list(games_dir.glob(f"game_{game_num:04d}_*.tg"))
            if matches:
                filtered_files.extend(matches)
                filtered_indices.append(game_num - 1)  # Convert to 0-based index
            else:
                print(f"Warning: No file found for game {game_num:04d}")
        game_files = sorted(filtered_files)
        # Update target indices to match
        targets = [targets[i] if i < len(targets) else [] for i in filtered_indices]
    
    # Apply start_from filter
    if start_from > 1:
        start_idx = start_from - 1
        game_files = game_files[start_idx:]
        targets = targets[start_idx:]
        print(f"Starting from game {start_from}")
    
    total_games = len(game_files)
    print(f"Processing {total_games} games with time bound {time_bound}")
    print("Using release build for better performance...")
    
    # Build release version first
    print("Building release version...")
    build_cmd = ['cargo', 'build', '--release']
    build_result = subprocess.run(build_cmd, cwd='/home/pete/ontime', capture_output=True, text=True)
    if build_result.returncode != 0:
        print(f"Warning: Release build failed, falling back to debug build")
    else:
        print("Release build completed successfully")
    
    successful_count = 0
    failed_count = 0
    skipped_count = 0
    
    for i, game_file in enumerate(game_files):
        game_index = (start_from - 1) + i if start_from > 1 else i
        
        if game_index >= len(targets):
            print(f"Warning: No target data for {game_file.name}")
            continue
            
        target_set = targets[game_index]
        game_num = game_index + 1
        
        print(f"[{game_num:4d}/{len(targets)}] {game_file.name}", end=" ")
        
        if not target_set:
            print("- SKIP (no targets)")
            result = {
                'game_number': game_num,
                'game_file': str(game_file),
                'targets': [],
                'success': False,
                'skipped': True,
                'winning_regions': {},
                'runtime': 0,
                'error': 'No target vertices'
            }
            skipped_count += 1
        else:
            print(f"- Targets: {','.join(target_set)}", end=" ", flush=True)
            
            result = run_ontime_command(game_file, target_set, time_bound, timeout)
            result.update({
                'game_number': game_num,
                'game_file': str(game_file),
                'targets': target_set,
                'skipped': False
            })
            
            if result['success']:
                print(f"✓ ({result['runtime']:.2f}s)", end="")
                w_regions = result['winning_regions']
                if w_regions:
                    w0 = w_regions.get(0, [])
                    wT = w_regions.get(time_bound, [])
                    print(f" W0={len(w0)}, W{time_bound}={len(wT)}")
                else:
                    print(" (no winning regions parsed)")
                successful_count += 1
            else:
                print(f"✗ ({result.get('error', 'Failed')})")
                failed_count += 1
        
        results.append(result)
        
        # Save intermediate results every 50 games or at the end
        if output_dir and (game_num % 50 == 0 or i == len(game_files) - 1):
            save_intermediate_results(results, output_dir, time_bound, successful_count, failed_count, skipped_count)
    
    return results

def save_intermediate_results(results, output_dir, time_bound, successful, failed, skipped):
    """Save intermediate results."""
    output_dir = Path(output_dir)
    output_dir.mkdir(exist_ok=True)
    
    # Save detailed CSV
    csv_file = output_dir / f"ontime_detailed_results_t{time_bound}.csv"
    save_detailed_csv(results, csv_file, time_bound)
    
    # Save progress summary
    summary_file = output_dir / f"progress_summary_t{time_bound}.txt"
    with open(summary_file, 'w') as f:
        f.write(f"Progress Summary (Time bound: {time_bound})\n")
        f.write(f"Processed: {len(results)} games\n")
        f.write(f"Successful: {successful}\n")
        f.write(f"Failed: {failed}\n")
        f.write(f"Skipped: {skipped}\n")
        
        if successful > 0:
            total_runtime = sum(r.get('runtime', 0) for r in results if not r.get('skipped', False))
            avg_runtime = total_runtime / (len(results) - skipped)
            f.write(f"Total runtime: {total_runtime:.2f}s\n")
            f.write(f"Average runtime: {avg_runtime:.3f}s per game\n")
    
    print(f"  → Results saved to {csv_file.name}")

def print_final_summary(results, time_bound):
    """Print final summary statistics."""
    total = len(results)
    successful = sum(1 for r in results if r.get('success', False))
    skipped = sum(1 for r in results if r.get('skipped', False))
    failed = total - successful - skipped
    
    total_runtime = sum(r.get('runtime', 0) for r in results if not r.get('skipped', False))
    
    print(f"\n{'='*60}")
    print(f"FINAL SUMMARY (Time bound: {time_bound})")
    print(f"{'='*60}")
    print(f"Total games processed: {total}")
    print(f"Successful: {successful}")
    print(f"Failed: {failed}")
    print(f"Skipped (no targets): {skipped}")
    print(f"Total runtime: {total_runtime:.2f}s ({total_runtime/60:.1f} minutes)")
    
    if successful > 0:
        avg_runtime = total_runtime / (total - skipped)
        print(f"Average runtime: {avg_runtime:.3f}s per game")
        
        # Analyze winning regions
        w0_stats = []
        wT_stats = []
        for r in results:
            if r.get('success', False) and r.get('winning_regions'):
                w_regions = r['winning_regions']
                w0_size = len(w_regions.get(0, []))
                wT_size = len(w_regions.get(time_bound, []))
                w0_stats.append(w0_size)
                wT_stats.append(wT_size)
        
        if w0_stats:
            print(f"Winning region sizes at t=0: min={min(w0_stats)}, max={max(w0_stats)}, avg={sum(w0_stats)/len(w0_stats):.1f}")
            print(f"Winning region sizes at t={time_bound}: min={min(wT_stats)}, max={max(wT_stats)}, avg={sum(wT_stats)/len(wT_stats):.1f}")

def main():
    parser = argparse.ArgumentParser(description='Enhanced batch processing for ontime solver')
    parser.add_argument('--games-dir', default='/home/pete/ontime/generated_games',
                       help='Directory containing game files')
    parser.add_argument('--targets-file', default='/home/pete/ontime/target_vertices.txt',
                       help='File containing target vertices')
    parser.add_argument('--output-dir', '-o', default='/home/pete/ontime/batch_results',
                       help='Output directory for results')
    parser.add_argument('--time-bound', '-t', type=int, default=10000,
                       help='Time bound for reachability (default: 10000)')
    parser.add_argument('--games', nargs='*', type=int,
                       help='Specific game numbers to process (e.g., 1 2 3)')
    parser.add_argument('--start-from', type=int, default=1,
                       help='Start processing from this game number (default: 1)')
    parser.add_argument('--timeout', type=int, default=120,
                       help='Timeout per game in seconds (default: 120)')
    parser.add_argument('--dry-run', action='store_true',
                       help='Show what would be done without running commands')
    
    args = parser.parse_args()
    
    # Validate inputs
    games_dir = Path(args.games_dir)
    targets_file = Path(args.targets_file)
    
    if not games_dir.exists():
        print(f"Error: Games directory {games_dir} does not exist")
        return 1
    
    if not targets_file.exists():
        print(f"Error: Target vertices file {targets_file} does not exist")
        return 1
    
    # Read target vertices
    print(f"Reading target vertices from {targets_file}")
    targets = read_target_vertices(targets_file)
    print(f"Found target data for {len(targets)} games")
    
    if args.dry_run:
        print("DRY RUN - showing first 5 games that would be processed")
        game_files = sorted(games_dir.glob('game_*.tg'))
        if args.games:
            game_files = [f for f in game_files if any(f"game_{num:04d}_" in f.name for num in args.games)]
        elif args.start_from > 1:
            game_files = game_files[args.start_from-1:]
        
        for i, game_file in enumerate(game_files[:5]):
            game_idx = (args.start_from - 1 + i) if args.start_from > 1 else i
            if game_idx < len(targets):
                target_set = targets[game_idx]
                if target_set:
                    print(f"Game {game_idx+1:4d}: {game_file.name} -> targets: {','.join(target_set)}")
                else:
                    print(f"Game {game_idx+1:4d}: {game_file.name} -> SKIP (no targets)")
        
        if len(game_files) > 5:
            print(f"... and {len(game_files) - 5} more games")
        return 0
    
    # Process games
    results = process_all_games(
        games_dir=games_dir,
        targets=targets,
        time_bound=args.time_bound,
        game_numbers=args.games,
        output_dir=args.output_dir,
        timeout=args.timeout,
        start_from=args.start_from
    )
    
    print_final_summary(results, args.time_bound)
    
    return 0

if __name__ == '__main__':
    exit(main())
