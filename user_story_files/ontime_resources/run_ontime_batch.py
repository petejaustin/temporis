#!/usr/bin/env python3
"""
Enhanced script to generate and run CLI commands for ontime solver on temporal games.
Includes batch processing, error handling, and result collection.
"""

import os
import subprocess
import argparse
import time
from pathlib import Path
import json

def read_target_vertices(target_file):
    """Read target vertices from file, return list of target sets."""
    targets = []
    with open(target_file, 'r') as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            if line:
                # Split comma-separated vertices
                target_set = [v.strip() for v in line.split(',')]
                targets.append(target_set)
            else:
                # Empty line means no targets for this game
                targets.append([])
    return targets

def run_ontime_command(game_file, targets, time_bound, timeout=60):
    """Run a single ontime command and return results."""
    if not targets:
        return {
            'success': False,
            'error': 'No target vertices for this game',
            'stdout': '',
            'stderr': '',
            'runtime': 0
        }
    
    target_str = ','.join(targets)
    cmd = ['cargo', 'run', '--', str(game_file), target_str, str(time_bound)]
    
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
        
        return {
            'success': result.returncode == 0,
            'stdout': result.stdout,
            'stderr': result.stderr,
            'returncode': result.returncode,
            'runtime': runtime,
            'command': ' '.join(cmd)
        }
    except subprocess.TimeoutExpired:
        return {
            'success': False,
            'error': f'Command timed out after {timeout} seconds',
            'stdout': '',
            'stderr': '',
            'runtime': timeout,
            'command': ' '.join(cmd)
        }
    except Exception as e:
        return {
            'success': False,
            'error': str(e),
            'stdout': '',
            'stderr': '',
            'runtime': time.time() - start_time,
            'command': ' '.join(cmd)
        }

def generate_and_run_commands(games_dir, targets, time_bound=10000, game_numbers=None, 
                            output_dir=None, timeout=60):
    """Generate and optionally run CLI commands for ontime solver."""
    
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
        for game_num in game_numbers:
            matches = list(games_dir.glob(f"game_{game_num:04d}_*.tg"))
            if matches:
                filtered_files.extend(matches)
            else:
                print(f"Warning: No file found for game {game_num:04d}")
        game_files = sorted(filtered_files)
    
    total_games = len(game_files)
    print(f"Processing {total_games} games with time bound {time_bound}")
    
    for i, game_file in enumerate(game_files):
        if i >= len(targets):
            print(f"Warning: No target data for {game_file.name}")
            continue
            
        target_set = targets[i]
        game_num = i + 1
        
        print(f"[{game_num:4d}/{total_games}] {game_file.name}", end=" ")
        
        if not target_set:
            print("- SKIP (no targets)")
            result = {
                'game_number': game_num,
                'game_file': str(game_file),
                'targets': [],
                'success': False,
                'skipped': True,
                'reason': 'No target vertices'
            }
        else:
            print(f"- Targets: {','.join(target_set)}", end=" ")
            
            result = run_ontime_command(game_file, target_set, time_bound, timeout)
            result.update({
                'game_number': game_num,
                'game_file': str(game_file),
                'targets': target_set,
                'skipped': False
            })
            
            if result['success']:
                print(f"✓ ({result['runtime']:.2f}s)")
            else:
                print(f"✗ ({result.get('error', 'Failed')})")
        
        results.append(result)
        
        # Save intermediate results
        if output_dir and (game_num % 10 == 0 or game_num == total_games):
            save_results(results, output_dir, time_bound)
    
    return results

def save_results(results, output_dir, time_bound):
    """Save results to files in output directory."""
    output_dir = Path(output_dir)
    output_dir.mkdir(exist_ok=True)
    
    # Save JSON results
    json_file = output_dir / f"ontime_results_t{time_bound}.json"
    with open(json_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    # Save CSV summary
    csv_file = output_dir / f"ontime_summary_t{time_bound}.csv"
    with open(csv_file, 'w') as f:
        f.write("game_number,game_file,targets,success,runtime,error\n")
        for r in results:
            targets_str = '"' + ','.join(r.get('targets', [])) + '"'
            error_str = r.get('error', r.get('stderr', '')).replace('"', '""')
            f.write(f"{r['game_number']},{r['game_file']},{targets_str},"
                   f"{r['success']},{r.get('runtime', 0):.3f},\"{error_str}\"\n")
    
    # Generate command script
    script_file = output_dir / f"ontime_commands_t{time_bound}.sh"
    with open(script_file, 'w') as f:
        f.write("#!/bin/bash\n")
        f.write("# Generated CLI commands for ontime solver\n")
        f.write(f"# Time bound: {time_bound}\n")
        f.write("cd /home/pete/ontime\n\n")
        
        for r in results:
            if r.get('skipped', False):
                f.write(f"# Game {r['game_number']:04d}: {Path(r['game_file']).name} - SKIPPED (no targets)\n\n")
            else:
                targets_str = ','.join(r['targets'])
                f.write(f"# Game {r['game_number']:04d}: {Path(r['game_file']).name} - Targets: {targets_str}\n")
                f.write(f"cargo run -- {r['game_file']} \"{targets_str}\" {time_bound}\n\n")
    
    os.chmod(script_file, 0o755)
    
    print(f"\nResults saved to {output_dir}/")
    print(f"  - JSON: {json_file.name}")
    print(f"  - CSV:  {csv_file.name}")
    print(f"  - Script: {script_file.name}")

def print_summary(results):
    """Print summary statistics."""
    total = len(results)
    successful = sum(1 for r in results if r.get('success', False))
    skipped = sum(1 for r in results if r.get('skipped', False))
    failed = total - successful - skipped
    
    total_runtime = sum(r.get('runtime', 0) for r in results if not r.get('skipped', False))
    
    print(f"\n=== Summary ===")
    print(f"Total games: {total}")
    print(f"Successful: {successful}")
    print(f"Skipped (no targets): {skipped}")
    print(f"Failed: {failed}")
    print(f"Total runtime: {total_runtime:.2f}s")
    if successful > 0:
        avg_runtime = total_runtime / (total - skipped)
        print(f"Average runtime: {avg_runtime:.3f}s per game")

def main():
    parser = argparse.ArgumentParser(description='Generate and run CLI commands for ontime solver')
    parser.add_argument('--games-dir', default='/home/pete/ontime/generated_games',
                       help='Directory containing game files')
    parser.add_argument('--targets-file', default='/home/pete/ontime/target_vertices.txt',
                       help='File containing target vertices')
    parser.add_argument('--output-dir', '-o', default='/home/pete/ontime/results',
                       help='Output directory for results')
    parser.add_argument('--time-bound', '-t', type=int, default=10000,
                       help='Time bound for reachability (default: 10000)')
    parser.add_argument('--games', nargs='*', type=int,
                       help='Specific game numbers to process (e.g., 1 2 3)')
    parser.add_argument('--timeout', type=int, default=60,
                       help='Timeout per game in seconds (default: 60)')
    parser.add_argument('--generate-only', action='store_true',
                       help='Only generate command scripts, do not run them')
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
        print("DRY RUN - no commands will be executed")
        # Just show what would be done for first few games
        game_files = sorted(games_dir.glob('game_*.tg'))
        if args.games:
            game_files = [f for f in game_files if any(f"game_{num:04d}_" in f.name for num in args.games)]
        
        for i, game_file in enumerate(game_files[:5]):
            if i < len(targets):
                target_set = targets[i]
                if target_set:
                    print(f"Would run: cargo run -- {game_file} \"{','.join(target_set)}\" {args.time_bound}")
                else:
                    print(f"Would skip: {game_file.name} (no targets)")
        
        if len(game_files) > 5:
            print(f"... and {len(game_files) - 5} more games")
        return 0
    
    # Generate and optionally run commands
    output_dir = None if args.generate_only else args.output_dir
    
    results = generate_and_run_commands(
        games_dir=games_dir,
        targets=targets,
        time_bound=args.time_bound,
        game_numbers=args.games,
        output_dir=output_dir,
        timeout=args.timeout
    )
    
    if not args.generate_only:
        print_summary(results)
    
    return 0

if __name__ == '__main__':
    exit(main())
