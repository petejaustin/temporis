#!/usr/bin/env python3
"""
Validate the generated temporal graph games by checking their syntax.
"""

import os
import re
from typing import List, Set

def validate_game_file(filepath: str) -> tuple[bool, List[str]]:
    """Validate a single game file and return (is_valid, errors)."""
    errors = []
    
    try:
        with open(filepath, 'r') as f:
            content = f.read()
    except Exception as e:
        return False, [f"Cannot read file: {e}"]
    
    lines = content.strip().split('\n')
    
    # Track nodes and edges
    nodes = set()
    edges = []
    
    for line_num, line in enumerate(lines, 1):
        line = line.strip()
        
        # Skip empty lines and comments
        if not line or line.startswith('//'):
            continue
        
        # Check node definitions
        node_match = re.match(r'node\s+(\w+):\s*(.+)', line)
        if node_match:
            node_id = node_match.group(1)
            attrs = node_match.group(2)
            
            nodes.add(node_id)
            
            # Check for required attributes
            if 'owner[' not in attrs:
                errors.append(f"Line {line_num}: Node {node_id} missing owner attribute")
            
            # Validate owner format
            owner_match = re.search(r'owner\[([01])\]', attrs)
            if not owner_match:
                errors.append(f"Line {line_num}: Node {node_id} has invalid owner format")
            
            continue
        
        # Check edge definitions
        edge_match = re.match(r'edge\s+(\w+)\s*->\s*(\w+)(?::\s*(.+))?', line)
        if edge_match:
            source = edge_match.group(1)
            target = edge_match.group(2)
            condition = edge_match.group(3)
            
            edges.append((source, target, condition))
            continue
        
        # If we get here, the line doesn't match expected format
        if line.strip():  # Only report non-empty lines
            errors.append(f"Line {line_num}: Unrecognized format: {line}")
    
    # Check that all edge endpoints reference valid nodes
    for source, target, condition in edges:
        if source not in nodes:
            errors.append(f"Edge references undefined source node: {source}")
        if target not in nodes:
            errors.append(f"Edge references undefined target node: {target}")
    
    # Basic sanity checks
    if not nodes:
        errors.append("No nodes defined")
    if not edges:
        errors.append("No edges defined")
    
    return len(errors) == 0, errors

def validate_all_games():
    """Validate all generated games."""
    games_dir = "generated_games"
    
    if not os.path.exists(games_dir):
        print(f"Error: {games_dir} directory not found")
        return
    
    game_files = [f for f in os.listdir(games_dir) if f.endswith('.tg')]
    game_files.sort()
    
    valid_count = 0
    total_count = len(game_files)
    
    print(f"Validating {total_count} game files...")
    
    for i, filename in enumerate(game_files):
        filepath = os.path.join(games_dir, filename)
        is_valid, errors = validate_game_file(filepath)
        
        if is_valid:
            valid_count += 1
        else:
            print(f"\nErrors in {filename}:")
            for error in errors:
                print(f"  - {error}")
        
        # Progress indicator
        if (i + 1) % 100 == 0:
            print(f"Validated {i + 1}/{total_count} files...")
    
    print(f"\nValidation complete:")
    print(f"  Valid games: {valid_count}/{total_count}")
    print(f"  Invalid games: {total_count - valid_count}")
    
    if valid_count == total_count:
        print("✅ All games are valid!")
    else:
        print("❌ Some games have errors.")
    
    # Show some statistics
    node_counts = []
    edge_counts = []
    
    for filename in game_files[:10]:  # Sample first 10 files
        filepath = os.path.join(games_dir, filename)
        try:
            with open(filepath, 'r') as f:
                content = f.read()
            
            nodes = len(re.findall(r'^node\s+', content, re.MULTILINE))
            edges = len(re.findall(r'^edge\s+', content, re.MULTILINE))
            
            node_counts.append(nodes)
            edge_counts.append(edges)
        except:
            pass
    
    if node_counts and edge_counts:
        print(f"\nSample statistics (first 10 games):")
        print(f"  Nodes: {min(node_counts)}-{max(node_counts)} (avg: {sum(node_counts)/len(node_counts):.1f})")
        print(f"  Edges: {min(edge_counts)}-{max(edge_counts)} (avg: {sum(edge_counts)/len(edge_counts):.1f})")

if __name__ == "__main__":
    validate_all_games()
