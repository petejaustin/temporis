#!/usr/bin/env python3
"""
Quick converter from .tg format to .dot format for testing purposes.
This creates a basic .dot file that temporis can process.
"""

import sys
import re

def convert_tg_to_dot(tg_file: str, dot_file: str):
    """Convert .tg file to .dot format."""
    
    with open(tg_file, 'r') as f:
        tg_content = f.read()
    
    dot_content = "digraph G {\n"
    
    # Parse nodes
    node_pattern = r'node\s+(\w+):\s*label\["([^"]+)"\],\s*owner\[(\d+)\]'
    for match in re.finditer(node_pattern, tg_content):
        node_id, label, owner = match.groups()
        dot_content += f'  {node_id} [label="{label}", owner="{owner}"];\n'
    
    # Parse edges and convert constraints
    edge_pattern = r'edge\s+(\w+)\s*->\s*(\w+)(?::\s*(.+))?'
    for match in re.finditer(edge_pattern, tg_content):
        source, target, constraint = match.groups()
        
        if constraint:
            # Convert common constraint patterns
            constraint = constraint.strip()
            if constraint == "(>= t 1)":
                constraint_str = "[constraint=\"t >= 1\"]"
            elif constraint == "(< t 15)":
                constraint_str = "[constraint=\"t < 15\"]"
            elif "(= (mod t" in constraint:
                # Extract modular constraint
                mod_match = re.search(r'\(= \(mod t (\d+)\) (\d+)\)', constraint)
                if mod_match:
                    mod_val, remainder = mod_match.groups()
                    constraint_str = f'[constraint=\"t mod {mod_val} == {remainder}\"]'
                else:
                    constraint_str = f'[constraint=\"{constraint}\"]'
            elif "(not (= (mod t" in constraint:
                # Extract negated modular constraint
                mod_match = re.search(r'\(not \(= \(mod t (\d+)\) (\d+)\)\)', constraint)
                if mod_match:
                    mod_val, remainder = mod_match.groups()
                    constraint_str = f'[constraint=\"t mod {mod_val} != {remainder}\"]'
                else:
                    constraint_str = f'[constraint=\"{constraint}\"]'
            elif "(and" in constraint:
                # Handle compound constraints
                constraint_str = f'[constraint=\"{constraint}\"]'
            else:
                constraint_str = f'[constraint=\"{constraint}\"]'
        else:
            constraint_str = ""
        
        dot_content += f'  {source} -> {target} {constraint_str};\n'
    
    dot_content += "}\n"
    
    with open(dot_file, 'w') as f:
        f.write(dot_content)

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 convert_tg_to_dot.py <input.tg> <output.dot>")
        sys.exit(1)
    
    tg_file = sys.argv[1]
    dot_file = sys.argv[2]
    
    convert_tg_to_dot(tg_file, dot_file)
    print(f"Converted {tg_file} -> {dot_file}")

if __name__ == "__main__":
    main()
