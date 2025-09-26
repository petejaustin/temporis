#!/usr/bin/env python3
"""
Convert ontime temporal graph games to temporis DOT format.

This script converts the .tg format used by ontime into the .dot format
expected by temporis, translating temporal conditions appropriately.
"""

import os
import re
import random
from typing import Dict, List, Tuple, Optional
from pathlib import Path

class OntimeToTemporisConverter:
    def __init__(self):
        self.node_mapping = {}
        
    def convert_temporal_condition(self, condition: str) -> str:
        """Convert ontime temporal condition to temporis constraint."""
        if not condition:
            return ""  # No constraint means always available
        
        condition = condition.strip()
        
        # Handle explicit time lists like (0,3,7,12)
        explicit_times = re.match(r'\(([0-9,\s]+)\)', condition)
        if explicit_times:
            times = [int(t.strip()) for t in explicit_times.group(1).split(',')]
            # Convert to disjunction of equalities
            time_constraints = [f"time == {t}" for t in times]
            return " || ".join(time_constraints)
        
        # Handle modular conditions like (= (mod t 3) 0) or ((= (mod t 3) 0))
        mod_condition = re.match(r'\(+\s*=\s*\(\s*mod\s+t\s+(\d+)\s*\)\s*(\d+)\s*\)+', condition)
        if mod_condition:
            modulus = int(mod_condition.group(1))
            remainder = int(mod_condition.group(2))
            return f"time % {modulus} == {remainder}"
        
        # Handle negated modular conditions like (not (= (mod t 3) 0)) or ((not (= (mod t 3) 0)))
        neg_mod = re.match(r'\(+\s*not\s*\(\s*=\s*\(\s*mod\s+t\s+(\d+)\s*\)\s*(\d+)\s*\)\s*\)+', condition)
        if neg_mod:
            modulus = int(neg_mod.group(1))
            remainder = int(neg_mod.group(2))
            return f"!(time % {modulus} == {remainder})"
        
        # Handle OR conditions like (or (= (mod t 2) 0) (= (mod t 3) 1))
        or_condition = re.match(r'\(\s*or\s+(.*)\)', condition)
        if or_condition:
            inner = or_condition.group(1)
            # Split by top-level expressions
            parts = self._split_logical_expressions(inner)
            converted_parts = []
            for part in parts:
                converted = self.convert_temporal_condition(f"({part.strip()})")
                if converted:
                    converted_parts.append(converted)
            return " || ".join(converted_parts) if converted_parts else ""
        
        # Handle AND conditions like (and (= (mod t 2) 0) (= (mod t 3) 1))
        and_condition = re.match(r'\(\s*and\s+(.*)\)', condition)
        if and_condition:
            inner = and_condition.group(1)
            parts = self._split_logical_expressions(inner)
            converted_parts = []
            for part in parts:
                converted = self.convert_temporal_condition(f"({part.strip()})")
                if converted:
                    converted_parts.append(converted)
            return " && ".join(converted_parts) if converted_parts else ""
        
        # Default: try to parse as a simple condition and make it more complex
        # This handles cases where we can't parse the condition directly
        print(f"Warning: Could not parse condition '{condition}', using default")
        return "time >= 0"  # Always available as fallback
    
    def _split_logical_expressions(self, text: str) -> List[str]:
        """Split logical expressions while respecting parentheses."""
        parts = []
        current = ""
        paren_count = 0
        
        for char in text:
            if char == '(':
                paren_count += 1
                current += char
            elif char == ')':
                paren_count -= 1
                current += char
            elif char == ' ' and paren_count == 0 and current.strip():
                # Space at top level, might be between expressions
                if current.strip().startswith('(') and current.strip().endswith(')'):
                    parts.append(current.strip())
                    current = ""
                else:
                    current += char
            else:
                current += char
        
        if current.strip():
            parts.append(current.strip())
        
        return parts
    
    def parse_ontime_game(self, filepath: str) -> Tuple[List[Dict], List[Dict]]:
        """Parse an ontime .tg file and return nodes and edges."""
        nodes = []
        edges = []
        
        with open(filepath, 'r') as f:
            content = f.read()
        
        # Parse nodes
        for match in re.finditer(r'node\s+(\w+):\s*(.+)', content):
            node_id = match.group(1)
            attrs = match.group(2)
            
            # Extract label and owner
            label_match = re.search(r'label\["([^"]+)"\]', attrs)
            owner_match = re.search(r'owner\[([01])\]', attrs)
            
            label = label_match.group(1) if label_match else node_id
            owner = int(owner_match.group(1)) if owner_match else 0
            
            nodes.append({
                'id': node_id,
                'label': label,
                'player': owner
            })
        
        # Parse edges
        for match in re.finditer(r'edge\s+(\w+)\s*->\s*(\w+)(?::\s*(.+))?', content):
            source = match.group(1)
            target = match.group(2)
            condition = match.group(3)
            
            edges.append({
                'source': source,
                'target': target,
                'condition': condition
            })
        
        return nodes, edges
    
    def convert_to_temporis_dot(self, nodes: List[Dict], edges: List[Dict], 
                               game_name: str, add_target: bool = True) -> str:
        """Convert parsed ontime game to temporis DOT format."""
        dot_content = f'digraph {game_name} {{\n'
        
        # Add nodes
        for i, node in enumerate(nodes):
            attrs = [
                f'name="{node["id"]}"',
                f'player={node["player"]}'
            ]
            
            # Randomly assign target status to some nodes (or use specific logic)
            if add_target and (i == len(nodes) - 1 or 
                              node['label'] in ['target', 't', 'goal'] or
                              random.random() < 0.1):  # 10% chance or specific labels
                attrs.append('target=1')
            
            attr_str = ', '.join(attrs)
            dot_content += f'    {node["id"]} [{attr_str}];\n'
        
        dot_content += '\n'
        
        # Add edges
        for edge in edges:
            constraint = self.convert_temporal_condition(edge['condition'])
            
            if constraint:
                dot_content += f'    {edge["source"]} -> {edge["target"]} [constraint="{constraint}"];\n'
            else:
                dot_content += f'    {edge["source"]} -> {edge["target"]};\n'
        
        dot_content += '}\n'
        return dot_content
    
    def convert_game_file(self, input_path: str, output_path: str):
        """Convert a single ontime game file to temporis format."""
        try:
            nodes, edges = self.parse_ontime_game(input_path)
            
            # Generate game name from filename
            game_name = Path(input_path).stem.replace('-', '_')
            
            # Convert to DOT format
            dot_content = self.convert_to_temporis_dot(nodes, edges, game_name)
            
            # Write output
            with open(output_path, 'w') as f:
                f.write(dot_content)
            
            return True
        except Exception as e:
            print(f"Error converting {input_path}: {e}")
            return False

def convert_all_games():
    """Convert all ontime games to temporis format."""
    converter = OntimeToTemporisConverter()
    
    # Input and output directories
    input_dir = "/home/sgpausti/temporis/user_story_files/ontime_resources/generated_games"
    output_dir = "/home/sgpausti/temporis/converted_games"
    
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)
    
    # Get all .tg files
    tg_files = [f for f in os.listdir(input_dir) if f.endswith('.tg')]
    tg_files.sort()
    
    print(f"Converting {len(tg_files)} ontime games to temporis format...")
    
    success_count = 0
    for i, filename in enumerate(tg_files):
        input_path = os.path.join(input_dir, filename)
        output_filename = filename.replace('.tg', '.dot')
        output_path = os.path.join(output_dir, output_filename)
        
        if converter.convert_game_file(input_path, output_path):
            success_count += 1
        
        if (i + 1) % 100 == 0:
            print(f"Converted {i + 1}/{len(tg_files)} files...")
    
    print(f"Conversion complete: {success_count}/{len(tg_files)} files converted successfully")
    
    # Create README for converted games
    readme_content = f"""# Converted Temporal Graph Games

This directory contains {success_count} temporal graph games converted from ontime format to temporis DOT format.

## Conversion Details

- **Source**: ontime .tg format with temporal conditions
- **Target**: temporis DOT format with Presburger arithmetic constraints
- **Node attributes**: name, player, target (randomly assigned)
- **Edge constraints**: Converted temporal conditions

## Format Changes

### Temporal Conditions
- `(= (mod t 3) 0)` → `time % 3 == 0`
- `(0,3,7,12)` → `time == 0 || time == 3 || time == 7 || time == 12`
- `(not (= (mod t 4) 0))` → `!(time % 4 == 0)`
- `(or (...) (...))` → `(...) || (...)`
- `(and (...) (...))` → `(...) && (...)`

### Node Attributes
- `owner[0]` → `player=0`
- `owner[1]` → `player=1`
- Target nodes assigned randomly or based on label hints

## Usage

Run any converted game with temporis:
```bash
./temporis converted_games/game_0001_chain_3.dot
./temporis converted_games/game_0201_branch_3_3.dot
```

## Game Types

- Chain graphs: `game_0001_*.dot` to `game_0200_*.dot`
- Branching graphs: `game_0201_*.dot` to `game_0400_*.dot`
- Cycle graphs: `game_0401_*.dot` to `game_0600_*.dot`
- Complex graphs: `game_0601_*.dot` to `game_1000_*.dot`
"""
    
    with open(os.path.join(output_dir, "README.md"), 'w') as f:
        f.write(readme_content)
    
    print(f"Created README.md in {output_dir}")

def test_conversion():
    """Test the conversion with a few sample games."""
    converter = OntimeToTemporisConverter()
    
    # Test cases
    test_cases = [
        ("(= (mod t 3) 0)", "time % 3 == 0"),
        ("(0,3,7)", "time == 0 || time == 3 || time == 7"),
        ("(not (= (mod t 4) 0))", "!(time % 4 == 0)"),
        ("", ""),
    ]
    
    print("Testing temporal condition conversion:")
    for input_cond, expected in test_cases:
        result = converter.convert_temporal_condition(input_cond)
        status = "✅" if result == expected else "❌"
        print(f"{status} '{input_cond}' → '{result}'")
        if result != expected:
            print(f"    Expected: '{expected}'")

if __name__ == "__main__":
    # Test conversion logic first
    test_conversion()
    print()
    
    # Convert all games
    convert_all_games()
