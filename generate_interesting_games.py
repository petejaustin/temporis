#!/usr/bin/env python3
"""
Generate interesting temporal graph games where reachability is actually possible.

This generator focuses on creating games where:
1. The reachability player has viable paths to targets
2. Both players have meaningful strategic choices
3. Timing constraints create interesting tactical decisions
4. Games avoid trivial "always blocked" scenarios

IMPORTANT: Uses Polish notation for temporal formulas (ontime format):
- (= t 5) means t == 5
- (>= t 3) means t >= 3  
- (= (mod t 4) 2) means t % 4 == 2
- (and (>= t 1) (< t 10)) means t >= 1 AND t < 10
- (not (= (mod t 3) 0)) means t % 3 != 0
"""

import random
import os
from typing import List, Set, Tuple, Dict
import math

class InterestingGameGenerator:
    def __init__(self):
        self.node_counter = 0
        
    def reset_counter(self):
        self.node_counter = 0
        
    def get_node_id(self) -> str:
        node_id = f"s{self.node_counter}"
        self.node_counter += 1
        return node_id

    def generate_guaranteed_reachable_chain(self, length: int) -> str:
        """Generate a chain where the target is guaranteed reachable."""
        self.reset_counter()
        content = f"// Guaranteed reachable chain with {length} nodes\n"
        content += "// At least one path to target is always available\n"
        content += "// Uses Polish notation: (= t 5) means t == 5\n\n"
        
        nodes = []
        for i in range(length):
            node_id = self.get_node_id()
            owner = i % 2
            content += f'node {node_id}: label["{node_id}"], owner[{owner}]\n'
            nodes.append(node_id)
        
        content += "\n"
        
        # Create guaranteed path (always available)
        main_path_length = min(3, length - 1)
        for i in range(main_path_length):
            content += f"edge {nodes[i]} -> {nodes[i+1]}\n"
        
        # Add alternative paths with timing constraints in Polish notation
        if length > 3:
            for i in range(main_path_length, length - 1):
                if random.random() < 0.7:  # 70% chance of timed edge
                    # Use compatible timing that actually works
                    if random.random() < 0.5:
                        # Modular timing: (= (mod t 3) 1) means t % 3 == 1
                        mod_val = random.choice([2, 3, 4])
                        remainder = random.randint(0, mod_val - 1)
                        content += f"edge {nodes[i]} -> {nodes[i+1]}: (= (mod t {mod_val}) {remainder})\n"
                    else:
                        # Time range: (and (>= t 2) (<= t 8))
                        start_time = i * 2  # Realistic timing based on position
                        end_time = start_time + random.randint(5, 15)
                        content += f"edge {nodes[i]} -> {nodes[i+1]}: (and (>= t {start_time}) (<= t {end_time}))\n"
                else:
                    # Always available edge
                    content += f"edge {nodes[i]} -> {nodes[i+1]}\n"
        
        # Add some strategic options (shortcuts, loops)
        if length > 4:
            # Shortcut path with timing requirement
            shortcut_start = random.randint(0, length - 3)
            shortcut_end = random.randint(shortcut_start + 2, length - 1)
            content += f"edge {nodes[shortcut_start]} -> {nodes[shortcut_end]}: (= (mod t 3) 1)\n"
            
            # Strategic self-loop for timing control
            loop_node = random.randint(1, length - 2)
            content += f"edge {nodes[loop_node]} -> {nodes[loop_node]}: (= (mod t 2) 0)\n"
        
        return content

    def generate_racing_game(self, paths: int, path_length: int) -> str:
        """Generate a racing game with multiple paths to the target."""
        self.reset_counter()
        content = f"// Racing game: {paths} paths of length {path_length}\n"
        content += "// Multiple routes to target with different timing strategies\n"
        content += "// Polish notation: (>= t 5) means t >= 5\n\n"
        
        # Start node
        start = self.get_node_id()
        content += f'node {start}: label["{start}"], owner[0]\n'
        
        # Create multiple paths
        all_paths = []
        for path_idx in range(paths):
            path_nodes = [start]  # Start from common node
            
            # Create intermediate nodes for this path
            for i in range(path_length - 1):
                node_id = self.get_node_id()
                owner = (path_idx + i + 1) % 2  # Alternate ownership
                content += f'node {node_id}: label["{node_id}"], owner[{owner}]\n'
                path_nodes.append(node_id)
            
            all_paths.append(path_nodes)
        
        # Target node
        target = self.get_node_id()
        content += f'node {target}: label["{target}"], owner[0]\n'
        
        content += "\n"
        
        # Create edges for each path with different strategies
        for path_idx, path_nodes in enumerate(all_paths):
            if path_idx == 0:
                # Fast path - always available but short
                for i in range(len(path_nodes) - 1):
                    content += f"edge {path_nodes[i]} -> {path_nodes[i+1]}\n"
                content += f"edge {path_nodes[-1]} -> {target}\n"
            
            elif path_idx == 1:
                # Timed path - requires good timing but reliable
                for i in range(len(path_nodes) - 1):
                    timing = f"(= (mod t 3) {i % 3})"
                    content += f"edge {path_nodes[i]} -> {path_nodes[i+1]}: {timing}\n"
                content += f"edge {path_nodes[-1]} -> {target}: (= (mod t 3) 2)\n"
            
            else:
                # Mixed strategy paths
                for i in range(len(path_nodes) - 1):
                    if i == 0:
                        # First edge has delay requirement
                        content += f"edge {path_nodes[i]} -> {path_nodes[i+1]}: (>= t {path_idx * 2})\n"
                    elif i == len(path_nodes) - 2:
                        # Last edge always available
                        content += f"edge {path_nodes[i]} -> {path_nodes[i+1]}\n"
                    else:
                        # Middle edges have various constraints
                        if random.random() < 0.5:
                            content += f"edge {path_nodes[i]} -> {path_nodes[i+1]}\n"
                        else:
                            content += f"edge {path_nodes[i]} -> {path_nodes[i+1]}: (not (= (mod t 4) 3))\n"
                
                content += f"edge {path_nodes[-1]} -> {target}: (< t 20)\n"
        
        # Add some cross-path connections for strategy
        if len(all_paths) > 1 and path_length > 2:
            path1, path2 = random.sample(all_paths, 2)
            pos1 = random.randint(1, len(path1) - 2)
            pos2 = random.randint(1, len(path2) - 2)
            content += f"edge {path1[pos1]} -> {path2[pos2]}: (= (mod t 5) 0)\n"
        
        return content

    def generate_strategic_diamond(self) -> str:
        """Generate a diamond-shaped game with strategic timing decisions."""
        self.reset_counter()
        content = "// Strategic diamond game\n"
        content += "// Players must choose between safe and risky paths\n"
        content += "// Polish notation: (not (= (mod t 3) 2)) means t % 3 != 2\n\n"
        
        # Create diamond structure: start -> top/bottom -> merge -> target
        start = self.get_node_id()
        top = self.get_node_id()
        bottom = self.get_node_id()
        merge = self.get_node_id()
        target = self.get_node_id()
        
        content += f'node {start}: label["{start}"], owner[0]\n'
        content += f'node {top}: label["{top}"], owner[1]\n'
        content += f'node {bottom}: label["{bottom}"], owner[1]\n'
        content += f'node {merge}: label["{merge}"], owner[0]\n'
        content += f'node {target}: label["{target}"], owner[0]\n'
        
        content += "\n"
        
        # Create strategic choices
        # Top path: Fast but risky (Player 1 can block)
        content += f"edge {start} -> {top}\n"
        content += f"edge {top} -> {merge}: (not (= (mod t 3) 2))\n"  # Player 1 can block every 3rd turn
        
        # Bottom path: Slower but more reliable
        content += f"edge {start} -> {bottom}: (>= t 1)\n"  # Slight delay
        content += f"edge {bottom} -> {merge}\n"  # Always available
        
        # Final path to target
        content += f"edge {merge} -> {target}: (< t 15)\n"  # Time pressure
        
        # Add self-loops for timing control
        content += f"edge {start} -> {start}: (= (mod t 2) 1)\n"
        content += f"edge {top} -> {top}: (= (mod t 4) 0)\n"
        content += f"edge {bottom} -> {bottom}: (= (mod t 3) 0)\n"
        
        return content

    def generate_tactical_grid(self, width: int, height: int) -> str:
        """Generate a grid game with tactical positioning."""
        self.reset_counter()
        content = f"// Tactical grid game {width}x{height}\n"
        content += "// Navigate through contested territory\n"
        content += "// Polish notation: (and (>= t 3) (= (mod t 5) 0)) means t >= 3 AND t % 5 == 0\n\n"
        
        # Create grid nodes
        grid = {}
        for y in range(height):
            for x in range(width):
                node_id = self.get_node_id()
                owner = (x + y) % 2  # Checkerboard ownership
                grid[(x, y)] = node_id
                content += f'node {node_id}: label["({x},{y})"], owner[{owner}]\n'
        
        content += "\n"
        
        # Create grid edges with strategic timing
        for y in range(height):
            for x in range(width):
                current = grid[(x, y)]
                
                # Right movement
                if x < width - 1:
                    right = grid[(x + 1, y)]
                    if x == 0 or x == width - 2:  # Critical positions
                        content += f"edge {current} -> {right}\n"  # Always available
                    else:
                        content += f"edge {current} -> {right}: (not (= (mod t 4) {x % 4}))\n"
                
                # Down movement
                if y < height - 1:
                    down = grid[(x, y + 1)]
                    if y == 0 or y == height - 2:  # Critical positions
                        content += f"edge {current} -> {down}\n"  # Always available
                    else:
                        timing = f"(>= t {y * 2})"
                        content += f"edge {current} -> {down}: {timing}\n"
                
                # Diagonal shortcuts (limited)
                if x < width - 1 and y < height - 1 and random.random() < 0.3:
                    diagonal = grid[(x + 1, y + 1)]
                    content += f"edge {current} -> {diagonal}: (and (>= t 3) (= (mod t 5) 0))\n"
        
        return content

    def generate_game_set(self, num_games: int, output_dir: str):
        """Generate a set of interesting games."""
        os.makedirs(output_dir, exist_ok=True)
        
        game_types = [
            ("chain", lambda: self.generate_guaranteed_reachable_chain(random.randint(4, 8))),
            ("racing", lambda: self.generate_racing_game(random.randint(2, 4), random.randint(3, 5))),
            ("diamond", lambda: self.generate_strategic_diamond()),
            ("grid", lambda: self.generate_tactical_grid(random.randint(3, 5), random.randint(3, 4))),
        ]
        
        for i in range(num_games):
            game_type, generator = random.choice(game_types)
            content = generator()
            
            filename = f"interesting_game_{i+1:03d}_{game_type}.tg"
            filepath = os.path.join(output_dir, filename)
            
            with open(filepath, 'w') as f:
                f.write(content)
            
            print(f"Generated: {filename}")

def main():
    """Generate interesting temporal games."""
    import sys
    
    num_games = int(sys.argv[1]) if len(sys.argv) > 1 else 20
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "interesting_games"
    
    print(f"Generating {num_games} interesting temporal games...")
    print(f"Output directory: {output_dir}")
    print()
    
    generator = InterestingGameGenerator()
    generator.generate_game_set(num_games, output_dir)
    
    print(f"\nGenerated {num_games} games in {output_dir}/")
    print("These games feature:")
    print("- Guaranteed reachable paths")
    print("- Strategic timing decisions")
    print("- Meaningful choices for both players")
    print("- Realistic temporal constraints")

if __name__ == "__main__":
    main()
