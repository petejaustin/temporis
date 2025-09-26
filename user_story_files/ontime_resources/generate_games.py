#!/usr/bin/env python3
"""
Generate 1000 temporal graph games for the ontime solver.

This script creates various types of temporal graphs with different structures:
- Simple chains and cycles
- Multi-path graphs
- Complex temporal conditions using modular arithmetic
- Explicit time-based availability lists
"""

import random
import os
from typing import List, Set, Tuple
import math

class TemporalGraphGenerator:
    def __init__(self):
        self.node_counter = 0
        
    def reset_counter(self):
        self.node_counter = 0
        
    def get_node_id(self) -> str:
        node_id = f"s{self.node_counter}"
        self.node_counter += 1
        return node_id

    def generate_simple_chain(self, length: int) -> str:
        """Generate a simple chain of nodes with alternating ownership."""
        self.reset_counter()
        content = f"// Simple chain graph with {length} nodes\n\n"
        
        nodes = []
        for i in range(length):
            node_id = self.get_node_id()
            owner = i % 2
            label = chr(ord('a') + i) if i < 26 else f"n{i}"
            content += f'node {node_id}: label["{label}"], owner[{owner}]\n'
            nodes.append(node_id)
        
        content += "\n"
        
        # Create chain edges with various temporal conditions
        for i in range(length - 1):
            # Some edges are always available, others have temporal conditions
            if random.random() < 0.3:
                # Always available edge
                content += f"edge {nodes[i]} -> {nodes[i+1]}\n"
            elif random.random() < 0.5:
                # Modular condition
                mod_val = random.randint(2, 5)
                remainder = random.randint(0, mod_val - 1)
                content += f"edge {nodes[i]} -> {nodes[i+1]}: (= (mod t {mod_val}) {remainder})\n"
            else:
                # Explicit time list
                times = sorted(random.sample(range(0, 20), random.randint(1, 5)))
                time_str = ",".join(map(str, times))
                content += f"edge {nodes[i]} -> {nodes[i+1]}: ({time_str})\n"
        
        # Add some self-loops and backward edges
        for i in range(length):
            if random.random() < 0.2:  # 20% chance of self-loop
                mod_val = random.randint(2, 4)
                content += f"edge {nodes[i]} -> {nodes[i]}: (= (mod t {mod_val}) 0)\n"
            
            if i > 0 and random.random() < 0.1:  # 10% chance of backward edge
                target = random.choice(nodes[:i])
                content += f"edge {nodes[i]} -> {target}: (not (= (mod t 3) 0))\n"
        
        return content

    def generate_branching_graph(self, depth: int, branching_factor: int) -> str:
        """Generate a tree-like branching graph."""
        self.reset_counter()
        content = f"// Branching graph with depth {depth} and branching factor {branching_factor}\n\n"
        
        # Generate nodes level by level
        levels = []
        for level in range(depth):
            level_nodes = []
            num_nodes = branching_factor ** level
            for i in range(num_nodes):
                node_id = self.get_node_id()
                owner = random.randint(0, 1)
                label = f"l{level}n{i}"
                content += f'node {node_id}: label["{label}"], owner[{owner}]\n'
                level_nodes.append(node_id)
            levels.append(level_nodes)
        
        content += "\n"
        
        # Generate edges between levels
        for level in range(depth - 1):
            for node in levels[level]:
                # Each node connects to branching_factor nodes in next level
                targets = levels[level + 1][
                    levels[level].index(node) * branching_factor:
                    (levels[level].index(node) + 1) * branching_factor
                ]
                
                for target in targets:
                    condition_type = random.choice(['always', 'mod', 'explicit', 'complex'])
                    
                    if condition_type == 'always':
                        content += f"edge {node} -> {target}\n"
                    elif condition_type == 'mod':
                        mod_val = random.randint(2, 6)
                        remainder = random.randint(0, mod_val - 1)
                        content += f"edge {node} -> {target}: (= (mod t {mod_val}) {remainder})\n"
                    elif condition_type == 'explicit':
                        max_time = 15
                        times = sorted(random.sample(range(0, max_time), random.randint(1, 4)))
                        time_str = ",".join(map(str, times))
                        content += f"edge {node} -> {target}: ({time_str})\n"
                    else:  # complex
                        # Create more complex formulas
                        if random.random() < 0.5:
                            mod1, mod2 = random.randint(2, 4), random.randint(2, 4)
                            content += f"edge {node} -> {target}: (or (= (mod t {mod1}) 0) (= (mod t {mod2}) 1))\n"
                        else:
                            mod_val = random.randint(3, 5)
                            content += f"edge {node} -> {target}: (not (= (mod t {mod_val}) 0))\n"
        
        return content

    def generate_cycle_graph(self, num_nodes: int) -> str:
        """Generate a cycle graph with temporal conditions."""
        self.reset_counter()
        content = f"// Cycle graph with {num_nodes} nodes\n\n"
        
        nodes = []
        for i in range(num_nodes):
            node_id = self.get_node_id()
            owner = random.randint(0, 1)
            label = f"c{i}"
            content += f'node {node_id}: label["{label}"], owner[{owner}]\n'
            nodes.append(node_id)
        
        content += "\n"
        
        # Create cycle edges
        for i in range(num_nodes):
            next_node = nodes[(i + 1) % num_nodes]
            
            # Various temporal conditions for cycle edges
            condition_type = random.choice(['mod', 'explicit', 'complex'])
            
            if condition_type == 'mod':
                mod_val = random.randint(2, 7)
                remainder = random.randint(0, mod_val - 1)
                content += f"edge {nodes[i]} -> {next_node}: (= (mod t {mod_val}) {remainder})\n"
            elif condition_type == 'explicit':
                # For cycles, use more spread out times
                max_time = 25
                times = sorted(random.sample(range(0, max_time), random.randint(2, 6)))
                time_str = ",".join(map(str, times))
                content += f"edge {nodes[i]} -> {next_node}: ({time_str})\n"
            else:  # complex
                if random.random() < 0.3:
                    mod1, mod2 = random.randint(2, 4), random.randint(3, 5)
                    content += f"edge {nodes[i]} -> {next_node}: (and (= (mod t {mod1}) 0) (not (= (mod t {mod2}) 0)))\n"
                elif random.random() < 0.6:
                    mod_val = random.randint(2, 5)
                    rem1, rem2 = random.sample(range(mod_val), 2)
                    content += f"edge {nodes[i]} -> {next_node}: (or (= (mod t {mod_val}) {rem1}) (= (mod t {mod_val}) {rem2}))\n"
                else:
                    mod_val = random.randint(3, 6)
                    content += f"edge {nodes[i]} -> {next_node}: (not (= (mod t {mod_val}) 0))\n"
        
        # Add some shortcuts and self-loops
        for i in range(num_nodes):
            # Self-loop
            if random.random() < 0.3:
                mod_val = random.randint(2, 4)
                content += f"edge {nodes[i]} -> {nodes[i]}: (= (mod t {mod_val}) 0)\n"
            
            # Shortcuts to non-adjacent nodes
            if random.random() < 0.2 and num_nodes > 3:
                possible_targets = [nodes[j] for j in range(num_nodes) 
                                 if j != i and j != (i + 1) % num_nodes and j != (i - 1) % num_nodes]
                if possible_targets:
                    target = random.choice(possible_targets)
                    times = sorted(random.sample(range(0, 15), random.randint(1, 3)))
                    time_str = ",".join(map(str, times))
                    content += f"edge {nodes[i]} -> {target}: ({time_str})\n"
        
        return content

    def generate_complex_graph(self, num_nodes: int) -> str:
        """Generate a complex graph with multiple paths and temporal conditions."""
        self.reset_counter()
        content = f"// Complex graph with {num_nodes} nodes\n\n"
        
        nodes = []
        for i in range(num_nodes):
            node_id = self.get_node_id()
            owner = random.randint(0, 1)
            # Use more varied labels
            if i == 0:
                label = "start"
            elif i == num_nodes - 1:
                label = "target"
            else:
                label = random.choice([f"n{i}", f"state{i}", chr(ord('a') + i % 26)])
            content += f'node {node_id}: label["{label}"], owner[{owner}]\n'
            nodes.append(node_id)
        
        content += "\n"
        
        # Generate edges with higher connectivity
        edge_probability = min(0.3, 6.0 / num_nodes)  # Adjust based on graph size
        
        for i in range(num_nodes):
            for j in range(num_nodes):
                if i != j and random.random() < edge_probability:
                    # Create various types of temporal conditions
                    condition_type = random.choice(['always', 'mod', 'explicit', 'not_mod', 'or_mod', 'and_mod'])
                    
                    if condition_type == 'always':
                        content += f"edge {nodes[i]} -> {nodes[j]}\n"
                    elif condition_type == 'mod':
                        mod_val = random.randint(2, 8)
                        remainder = random.randint(0, mod_val - 1)
                        content += f"edge {nodes[i]} -> {nodes[j]}: (= (mod t {mod_val}) {remainder})\n"
                    elif condition_type == 'explicit':
                        max_time = random.randint(15, 30)
                        num_times = random.randint(1, 5)
                        times = sorted(random.sample(range(0, max_time), num_times))
                        time_str = ",".join(map(str, times))
                        content += f"edge {nodes[i]} -> {nodes[j]}: ({time_str})\n"
                    elif condition_type == 'not_mod':
                        mod_val = random.randint(2, 6)
                        remainder = random.randint(0, mod_val - 1)
                        content += f"edge {nodes[i]} -> {nodes[j]}: (not (= (mod t {mod_val}) {remainder}))\n"
                    elif condition_type == 'or_mod':
                        mod1, mod2 = random.randint(2, 5), random.randint(2, 5)
                        rem1, rem2 = random.randint(0, mod1 - 1), random.randint(0, mod2 - 1)
                        content += f"edge {nodes[i]} -> {nodes[j]}: (or (= (mod t {mod1}) {rem1}) (= (mod t {mod2}) {rem2}))\n"
                    else:  # and_mod
                        mod1, mod2 = random.randint(2, 4), random.randint(3, 5)
                        rem1, rem2 = random.randint(0, mod1 - 1), random.randint(0, mod2 - 1)
                        content += f"edge {nodes[i]} -> {nodes[j]}: (and (= (mod t {mod1}) {rem1}) (= (mod t {mod2}) {rem2}))\n"
        
        return content

def generate_all_games():
    """Generate 1000 different temporal graph games."""
    generator = TemporalGraphGenerator()
    
    # Create output directory
    os.makedirs("generated_games", exist_ok=True)
    
    game_types = []
    
    # Generate different types of games
    for i in range(1000):
        if i < 200:  # Simple chains (20%)
            length = random.randint(3, 12)
            content = generator.generate_simple_chain(length)
            game_types.append(f"chain_{length}")
        elif i < 400:  # Branching graphs (20%)
            depth = random.randint(2, 4)
            branching = random.randint(2, 3)
            content = generator.generate_branching_graph(depth, branching)
            game_types.append(f"branch_{depth}_{branching}")
        elif i < 600:  # Cycle graphs (20%)
            num_nodes = random.randint(3, 10)
            content = generator.generate_cycle_graph(num_nodes)
            game_types.append(f"cycle_{num_nodes}")
        else:  # Complex graphs (40%)
            num_nodes = random.randint(4, 15)
            content = generator.generate_complex_graph(num_nodes)
            game_types.append(f"complex_{num_nodes}")
        
        # Write to file
        filename = f"generated_games/game_{i+1:04d}_{game_types[-1]}.tg"
        with open(filename, 'w') as f:
            f.write(content)
        
        if (i + 1) % 100 == 0:
            print(f"Generated {i + 1} games...")
    
    print(f"Successfully generated 1000 games in the 'generated_games' directory")
    
    # Generate a summary file
    with open("generated_games/README.md", 'w') as f:
        f.write("# Generated Temporal Graph Games\n\n")
        f.write("This directory contains 1000 generated temporal graph games for the ontime solver.\n\n")
        f.write("## Game Types\n\n")
        f.write("- **Chain graphs (games 1-200)**: Simple linear chains with temporal conditions\n")
        f.write("- **Branching graphs (games 201-400)**: Tree-like structures with multiple paths\n")
        f.write("- **Cycle graphs (games 401-600)**: Circular structures with temporal loops\n")
        f.write("- **Complex graphs (games 601-1000)**: Dense graphs with varied connectivity\n\n")
        f.write("## Usage\n\n")
        f.write("Run any game with the ontime solver:\n")
        f.write("```bash\n")
        f.write("cargo run -- generated_games/game_0001_chain_5.tg \"s0,s4\" 100\n")
        f.write("```\n\n")
        f.write("## Temporal Conditions\n\n")
        f.write("The games use various temporal conditions:\n")
        f.write("- Modular arithmetic: `(= (mod t 3) 0)`\n")
        f.write("- Explicit time lists: `(0,3,7,12)`\n")
        f.write("- Boolean combinations: `(or (= (mod t 2) 0) (= (mod t 3) 1))`\n")
        f.write("- Negations: `(not (= (mod t 4) 0))`\n")

if __name__ == "__main__":
    generate_all_games()
