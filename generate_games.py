#!/usr/bin/env python3
"""
Benchmark Game Generator
Generates 150 temporal games for performance testing: 15 games each for sizes 10-100 vertices
Uses flat directory structure in /benchmark/
"""

import random
import os
from typing import List, Tuple

class BenchmarkGameGenerator:
    def __init__(self):
        self.node_counter = 0
        
    def generate_node_name(self, index: int) -> str:
        return f"v{index}"
    
    def generate_benchmark_game(self, num_nodes: int, game_id: int) -> Tuple[str, str, int]:
        """Generate a benchmark game with specified number of nodes"""
        
        # Reset for this game
        nodes = []
        
        # Create nodes with random owners
        for i in range(num_nodes):
            name = self.generate_node_name(i)
            owner = random.randint(0, 1)
            nodes.append((name, owner))
        
        # Select 10-20% of nodes as targets (at least 1)
        target_ratio = random.uniform(0.1, 0.2)
        num_targets = max(1, int(num_nodes * target_ratio))
        target_indices = random.sample(range(num_nodes), num_targets)
        targets = [nodes[i][0] for i in target_indices]
        
        # Generate edges - approximately 1.5x to 3x the number of nodes
        edge_multiplier = random.uniform(1.5, 3.0)
        num_edges = int(num_nodes * edge_multiplier)
        
        edges = []
        node_names = [n[0] for n in nodes]
        
        # Ensure connectivity: each node has at least one outgoing edge
        for i, (node_name, _) in enumerate(nodes):
            # Add at least one edge to a random target
            target_node = random.choice(node_names)
            constraint = self._generate_temporal_constraint()
            edges.append((node_name, target_node, constraint))
        
        # Add additional random edges
        for _ in range(num_edges - num_nodes):
            src = random.choice(node_names)
            dst = random.choice(node_names)
            constraint = self._generate_temporal_constraint()
            edges.append((src, dst, constraint))
        
        # Generate reasonable time bound (scales with graph size)
        time_bound = random.randint(max(5, num_nodes // 10), max(10, num_nodes // 5))
        
        # Create .tg format (ontime)
        tg_content = f"// Benchmark game {game_id} - {num_nodes} vertices\n"
        tg_content += f"// time_bound: {time_bound}\n"
        tg_content += f"// targets: {','.join(targets)}\n"
        for name, owner in nodes:
            tg_content += f"node {name}: owner[{owner}]\n"
        tg_content += "\n"
        for src, dst, constraint in edges:
            if constraint:
                tg_content += f"edge {src} -> {dst}: {constraint}\n"
            else:
                tg_content += f"edge {src} -> {dst}\n"
        
        # Create .dot format (temporis)
        dot_content = f"// Benchmark game {game_id} - {num_nodes} vertices\n"
        dot_content += "digraph G {\n"
        for name, owner in nodes:
            is_target = name in targets
            if is_target:
                dot_content += f'    {name} [name="{name}", player={owner}, target=1];\n'
            else:
                dot_content += f'    {name} [name="{name}", player={owner}];\n'
        dot_content += "\n"
        for src, dst, constraint in edges:
            if constraint:
                temporis_constraint = self._convert_constraint_to_temporis(constraint)
                dot_content += f'    {src} -> {dst} [constraint="{temporis_constraint}"];\n'
            else:
                dot_content += f'    {src} -> {dst};\n'
        dot_content += "}\n"
        
        return tg_content, dot_content, time_bound
    
    def _generate_temporal_constraint(self) -> str:
        """Generate a random temporal constraint in ontime format"""
        constraint_types = [
            None,  # No constraint (30% chance)
            "equality",
            "modulo", 
            "greater_equal",
            "less_equal",
            "complex"
        ]
        
        # Weight towards simpler constraints for performance
        weights = [0.3, 0.2, 0.2, 0.15, 0.1, 0.05]
        constraint_type = random.choices(constraint_types, weights=weights)[0]
        
        if constraint_type is None:
            return None
        elif constraint_type == "equality":
            time = random.randint(0, 20)
            return f"(= t {time})"
        elif constraint_type == "modulo":
            mod = random.randint(2, 6)
            val = random.randint(0, mod-1)
            return f"(= (mod t {mod}) {val})"
        elif constraint_type == "greater_equal":
            time = random.randint(1, 15)
            return f"(>= t {time})"
        elif constraint_type == "less_equal":
            time = random.randint(5, 25)
            return f"(<= t {time})"
        elif constraint_type == "complex":
            # More complex constraints for stress testing
            if random.random() < 0.5:
                mod1, val1 = random.randint(2, 4), random.randint(0, 1)
                mod2, val2 = random.randint(3, 5), random.randint(0, 2)
                return f"(or (= (mod t {mod1}) {val1}) (= (mod t {mod2}) {val2}))"
            else:
                t1, t2 = random.randint(1, 10), random.randint(15, 25)
                return f"(and (>= t {t1}) (<= t {t2}))"
        
        return None
    
    def _convert_constraint_to_temporis(self, ontime_constraint: str) -> str:
        """Convert ontime constraint format to temporis format"""
        if not ontime_constraint:
            return ""
        
        constraint = ontime_constraint
        
        # Handle simple cases first
        if constraint.startswith("(= t "):
            # (= t 5) -> time == 5
            return constraint.replace("(= t ", "time == ").replace(")", "")
        elif constraint.startswith("(>= t "):
            # (>= t 5) -> time >= 5
            return constraint.replace("(>= t ", "time >= ").replace(")", "")
        elif constraint.startswith("(<= t "):
            # (<= t 5) -> time <= 5
            return constraint.replace("(<= t ", "time <= ").replace(")", "")
        elif constraint.startswith("(= (mod t "):
            # (= (mod t 3) 1) -> time % 3 == 1
            import re
            match = re.match(r'\(= \(mod t (\d+)\) (\d+)\)', constraint)
            if match:
                mod_val, result = match.groups()
                return f"time % {mod_val} == {result}"
        elif constraint.startswith("(or "):
            # (or (= (mod t 2) 0) (= (mod t 3) 1)) -> (time % 2 == 0) || (time % 3 == 1)
            import re
            inner = constraint[4:-1]  # Remove "(or " and ")"
            parts = []
            if "(= (mod t" in inner:
                mod_parts = re.findall(r'\(= \(mod t (\d+)\) (\d+)\)', inner)
                for mod_val, result in mod_parts:
                    parts.append(f"time % {mod_val} == {result}")
            if parts:
                return "(" + ") || (".join(parts) + ")"
        elif constraint.startswith("(and "):
            # (and (>= t 1) (<= t 10)) -> (time >= 1) && (time <= 10)
            import re
            inner = constraint[5:-1]  # Remove "(and " and ")"
            parts = []
            ge_match = re.search(r'\(>= t (\d+)\)', inner)
            le_match = re.search(r'\(<= t (\d+)\)', inner)
            if ge_match:
                parts.append(f"time >= {ge_match.group(1)}")
            if le_match:
                parts.append(f"time <= {le_match.group(1)}")
            if parts:
                return "(" + ") && (".join(parts) + ")"
        
        # Fallback: return simplified version
        return constraint.replace("(", "").replace(")", "").replace("= t", "time ==").replace(">= t", "time >=").replace("<= t", "time <=")

def generate_all_benchmark_games():
    """Generate all benchmark games in flat directory structure"""
    
    generator = BenchmarkGameGenerator()
    
    # Create benchmark directory
    benchmark_dir = "/home/pete/temporis/benchmark"
    os.makedirs(benchmark_dir, exist_ok=True)
    
    # Clean existing files
    for file in os.listdir(benchmark_dir):
        if file.startswith("test"):
            os.remove(os.path.join(benchmark_dir, file))
    
    sizes = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
    
    print("Generating benchmark games with flat directory structure...")
    print("=" * 60)
    
    game_id = 1
    total_games = len(sizes) * 15
    
    for size in sizes:
        print(f"Generating {size:3d} vertex games... ", end="", flush=True)
        
        for i in range(15):
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
    
    print("=" * 60)
    print(f"Generated {total_games} benchmark games in flat structure")
    print(f"Sizes: {sizes}")
    print(f"Files created in {benchmark_dir}/")
    print(f"  - test001.tg/dot/meta through test150.tg/dot/meta")

if __name__ == "__main__":
    # Set random seed for reproducible benchmarks
    random.seed(42)
    generate_all_benchmark_games()
