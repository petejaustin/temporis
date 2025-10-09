#!/usr/bin/env python3
"""
Ontime vs Temporis Comparison Script

Compares winning regions between ontime and temporis solvers with clear output.
"""

import subprocess
import re
import sys
from typing import Set, Tuple, Optional

class GameResult:
    def __init__(self, success: bool, w0_region: set = None, w_final_region: set = None, all_nodes: set = None, error: str = ""):
        self.success = success
        self.w0_region = w0_region or set()
        self.w_final_region = w_final_region or set()
        self.all_nodes = all_nodes or set()
        self.error = error
    
    @property
    def player1_region(self) -> set:
        """Player 1 winning region is the complement of W_0"""
        return self.all_nodes - self.w0_region

class TemporisResult:
    def __init__(self, success: bool, p0_region: set = None, p1_region: set = None, error: str = ""):
        self.success = success
        self.p0_region = p0_region or set()
        self.p1_region = p1_region or set()
        self.error = error

def run_ontime(game_file: str, targets: str, time_bound: int = 10000) -> GameResult:
    """Run ontime solver and parse results."""
    cmd = ["/home/pete/ontime/target/debug/ontime", game_file, targets, str(time_bound)]
    
    try:
        # First, extract all nodes from the .tg file
        all_nodes = set()
        try:
            with open(game_file, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line and not line.startswith('//'):
                        # Parse node declarations: node s0: label["s0"], owner[0]
                        if line.startswith('node '):
                            node_match = re.match(r'node\s+(\w+):', line)
                            if node_match:
                                all_nodes.add(node_match.group(1))
                        # Parse edge declarations: edge s0 -> s1: (= t 5)
                        elif line.startswith('edge '):
                            edge_match = re.match(r'edge\s+(\w+)\s*->\s*(\w+):', line)
                            if edge_match:
                                all_nodes.add(edge_match.group(1))
                                all_nodes.add(edge_match.group(2))
        except Exception as e:
            return GameResult(False, error=f"Failed to parse game file: {e}")
        
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        
        if result.returncode != 0:
            return GameResult(False, error=result.stderr)
        
        # Parse ontime output format:
        # W_10000 = {"s0", "s1"}
        # W_0 = {}
        output = result.stdout.strip()
        w0_region = set()
        w_final_region = set()
        
        # Extract W_0
        w0_pattern = r'W_0\s*=\s*\{([^}]*)\}'
        w0_match = re.search(w0_pattern, output)
        if w0_match:
            nodes_str = w0_match.group(1)
            nodes = re.findall(r'"([^"]+)"', nodes_str)
            w0_region = set(nodes)
        
        # Extract W_<time_bound>
        w_final_pattern = rf'W_{time_bound}\s*=\s*\{{([^}}]*)\}}'
        w_final_match = re.search(w_final_pattern, output)
        if w_final_match:
            nodes_str = w_final_match.group(1)
            nodes = re.findall(r'"([^"]+)"', nodes_str)
            w_final_region = set(nodes)
        
        return GameResult(True, w0_region, w_final_region, all_nodes)
    
    except subprocess.TimeoutExpired:
        return GameResult(False, error="Timeout")
    except Exception as e:
        return GameResult(False, error=str(e))

def run_temporis(game_file: str) -> TemporisResult:
    """Run temporis solver and parse results."""
    cmd = ["./temporis", game_file]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30, cwd="/home/pete/temporis")
        
        if result.returncode != 0:
            return TemporisResult(False, error=result.stderr)
        
        # Parse temporis output format:
        # Player 0:
        # Winning regions: 
        #   {}
        # Player 1:
        # Winning regions:
        #   {s0, s1}
        output = result.stdout.strip()
        p0_region = set()
        p1_region = set()
        
        # Split by "Player" to get sections
        player_sections = re.split(r'Player (\d+):', output)
        
        for i in range(1, len(player_sections), 2):
            player_num = int(player_sections[i])
            player_section = player_sections[i+1] if i+1 < len(player_sections) else ""
            
            # Extract winning regions from this player's section
            regions_match = re.search(r'Winning regions:\s*\{([^}]*)\}', player_section)
            if regions_match:
                nodes_str = regions_match.group(1).strip()
                if nodes_str:
                    # Split by comma and clean up
                    nodes = [node.strip() for node in nodes_str.split(',') if node.strip()]
                    if player_num == 0:
                        p0_region = set(nodes)
                    elif player_num == 1:
                        p1_region = set(nodes)
        
        return TemporisResult(True, p0_region, p1_region)
    
    except subprocess.TimeoutExpired:
        return TemporisResult(False, error="Timeout")
    except Exception as e:
        return TemporisResult(False, error=str(e))

def compare_games(ontime_file: str, temporis_file: str, targets: str, time_bound: int = 10000):
    """Compare ontime and temporis on given game files."""
    
    print("=" * 80)
    print("ONTIME vs TEMPORIS COMPARISON")
    print("=" * 80)
    print(f"Ontime file:   {ontime_file}")
    print(f"Temporis file: {temporis_file}")
    print(f"Targets:       {targets}")
    print(f"Time bound:    {time_bound}")
    print()
    
    # Run ontime
    print("Running ontime...", end=" ", flush=True)
    ontime_result = run_ontime(ontime_file, targets, time_bound)
    if ontime_result.success:
        print(f"✓")
    else:
        print(f"✗ {ontime_result.error}")
    
    # Run temporis
    print("Running temporis...", end=" ", flush=True)
    temporis_result = run_temporis(temporis_file)
    if temporis_result.success:
        print(f"✓")
    else:
        print(f"✗ {temporis_result.error}")
    
    print()
    
    # Display detailed results
    print("ONTIME RESULTS:")
    print("-" * 40)
    if ontime_result.success:
        w0_sorted = sorted(ontime_result.w0_region) if ontime_result.w0_region else []
        w_final_sorted = sorted(ontime_result.w_final_region) if ontime_result.w_final_region else []
        p1_sorted = sorted(ontime_result.player1_region) if ontime_result.player1_region else []
        print(f"W_0 (winning at time 0): {{{', '.join(w0_sorted) if w0_sorted else 'empty'}}}")
        print(f"Player 1 winning (complement of W_0): {{{', '.join(p1_sorted) if p1_sorted else 'empty'}}}")
        print(f"W_{time_bound} (target set):     {{{', '.join(w_final_sorted) if w_final_sorted else 'empty'}}}")
    else:
        print(f"ERROR: {ontime_result.error}")
    
    print()
    print("TEMPORIS RESULTS:")
    print("-" * 40)
    if temporis_result.success:
        p0_sorted = sorted(temporis_result.p0_region) if temporis_result.p0_region else []
        p1_sorted = sorted(temporis_result.p1_region) if temporis_result.p1_region else []
        print(f"Player 0 (reachability) winning: {{{', '.join(p0_sorted) if p0_sorted else 'empty'}}}")
        print(f"Player 1 (safety) winning:       {{{', '.join(p1_sorted) if p1_sorted else 'empty'}}}")
    else:
        print(f"ERROR: {temporis_result.error}")
    
    print()
    
    # Compare W_0 vs Player 0 winning regions
    print("COMPARISON (W_0 vs Player 0):")
    print("-" * 40)
    if ontime_result.success and temporis_result.success:
        if ontime_result.w0_region == temporis_result.p0_region:
            print("✅ MATCH: W_0 equals Player 0 winning regions")
        else:
            print("❌ DIFFERENT: W_0 differs from Player 0 winning regions")
            print()
            
            only_ontime = ontime_result.w0_region - temporis_result.p0_region
            only_temporis = temporis_result.p0_region - ontime_result.w0_region
            
            if only_ontime:
                print(f"Only in ontime W_0:   {{{', '.join(sorted(only_ontime))}}}")
            if only_temporis:
                print(f"Only in temporis P0:  {{{', '.join(sorted(only_temporis))}}}")
            
            common = ontime_result.w0_region & temporis_result.p0_region
            if common:
                print(f"In both:              {{{', '.join(sorted(common))}}}")
    else:
        print("Cannot compare due to errors")
    
    print("=" * 80)

def main():
    if len(sys.argv) < 4:
        print("Usage: python3 compare_games.py <ontime_file.tg> <temporis_file.dot> <targets> [time_bound]")
        print()
        print("Examples:")
        print("  python3 compare_games.py game.tg game.dot s2 10")
        print("  python3 compare_games.py game.tg game.dot s0,s1 10000")
        sys.exit(1)
    
    ontime_file = sys.argv[1]
    temporis_file = sys.argv[2]
    targets = sys.argv[3]
    time_bound = int(sys.argv[4]) if len(sys.argv) > 4 else 10000
    
    compare_games(ontime_file, temporis_file, targets, time_bound)

if __name__ == "__main__":
    main()
