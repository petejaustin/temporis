#!/usr/bin/env python3
"""
Script to randomize winning regions in temporal game files.
This script modifies all .dot files in the converted_games directory
by randomly reassigning the target=1 attributes to different vertices.
"""

import os
import re
import random
import argparse
from pathlib import Path

def parse_dot_file(content):
    """Parse a DOT file and extract vertex information."""
    vertices = []
    edges = []
    
    # Find all vertex declarations
    vertex_pattern = r'(\w+)\s*\[([^\]]+)\];'
    vertex_matches = re.findall(vertex_pattern, content)
    
    for vertex_name, attributes in vertex_matches:
        attr_dict = {}
        # Parse attributes
        attr_pattern = r'(\w+)=([^,\]]+)'
        attr_matches = re.findall(attr_pattern, attributes)
        
        for attr_name, attr_value in attr_matches:
            attr_dict[attr_name.strip()] = attr_value.strip().strip('"')
        
        vertices.append((vertex_name, attr_dict))
    
    # Find all edges (we'll keep them as-is)
    edge_pattern = r'(\w+)\s*->\s*(\w+)([^;]*);'
    edge_matches = re.findall(edge_pattern, content)
    edges = edge_matches
    
    return vertices, edges

def randomize_targets(vertices, target_probability=0.3):
    """Randomly assign target=1 to vertices based on probability."""
    # Remove all existing target attributes
    for vertex_name, attrs in vertices:
        if 'target' in attrs:
            del attrs['target']
    
    # Randomly assign target=1 to some vertices
    for vertex_name, attrs in vertices:
        if random.random() < target_probability:
            attrs['target'] = '1'

def format_attributes(attr_dict):
    """Format vertex attributes as a string."""
    attrs = []
    for key, value in attr_dict.items():
        if key in ['name', 'player']:  # These don't need quotes
            if key == 'name':
                attrs.append(f'{key}="{value}"')
            else:
                attrs.append(f'{key}={value}')
        else:
            attrs.append(f'{key}={value}')
    return ', '.join(attrs)

def reconstruct_dot_file(vertices, edges, game_name):
    """Reconstruct the DOT file content."""
    lines = [f'digraph {game_name} {{']
    
    # Add vertices
    for vertex_name, attrs in vertices:
        attr_str = format_attributes(attrs)
        lines.append(f'    {vertex_name} [{attr_str}];')
    
    # Add empty line
    lines.append('')
    
    # Add edges (reconstruct from original content)
    return lines

def process_single_file(file_path, target_probability=0.3, seed=None):
    """Process a single DOT file to randomize winning regions."""
    if seed is not None:
        random.seed(seed)
    
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Extract game name from digraph declaration
    game_name_match = re.search(r'digraph\s+(\w+)', content)
    game_name = game_name_match.group(1) if game_name_match else 'game'
    
    vertices, edges = parse_dot_file(content)
    randomize_targets(vertices, target_probability)
    
    # Reconstruct the file by preserving the original structure
    # but updating only the vertex declarations
    lines = content.split('\n')
    new_lines = []
    
    # Find the digraph line
    digraph_found = False
    vertex_section = False
    
    for line in lines:
        stripped = line.strip()
        
        if re.match(r'digraph\s+\w+\s*\{', stripped):
            new_lines.append(line)
            digraph_found = True
            vertex_section = True
            continue
        
        # Check if this is a vertex declaration
        if vertex_section and re.match(r'\s*\w+\s*\[.*\];', stripped):
            # Find the corresponding vertex in our parsed data
            vertex_match = re.match(r'\s*(\w+)\s*\[', stripped)
            if vertex_match:
                vertex_name = vertex_match.group(1)
                # Find this vertex in our vertices list
                for v_name, v_attrs in vertices:
                    if v_name == vertex_name:
                        attr_str = format_attributes(v_attrs)
                        new_lines.append(f'    {vertex_name} [{attr_str}];')
                        break
                continue
        
        # Check if we've moved past the vertex section
        if vertex_section and (stripped.startswith('s') and '->' in stripped):
            vertex_section = False
        
        # Keep all other lines as-is
        new_lines.append(line)
    
    # Write back to file
    with open(file_path, 'w') as f:
        f.write('\n'.join(new_lines))

def main():
    parser = argparse.ArgumentParser(description='Randomize winning regions in temporal game files')
    parser.add_argument('directory', nargs='?', 
                       default='/home/pete/temporis/user_story_files/temporis_resources/converted_games',
                       help='Directory containing .dot game files')
    parser.add_argument('--probability', '-p', type=float, default=0.3,
                       help='Probability of a vertex being a target (default: 0.3)')
    parser.add_argument('--seed', '-s', type=int, default=42,
                       help='Random seed for reproducibility (default: 42)')
    parser.add_argument('--dry-run', action='store_true',
                       help='Show what would be done without making changes')
    
    args = parser.parse_args()
    
    directory = Path(args.directory)
    if not directory.exists():
        print(f"Error: Directory {directory} does not exist")
        return 1
    
    # Find all .dot files
    dot_files = list(directory.glob('*.dot'))
    if not dot_files:
        print(f"No .dot files found in {directory}")
        return 1
    
    print(f"Found {len(dot_files)} .dot files")
    print(f"Target probability: {args.probability}")
    print(f"Random seed: {args.seed}")
    
    if args.dry_run:
        print("DRY RUN - no files will be modified")
        return 0
    
    # Set the random seed for reproducibility
    random.seed(args.seed)
    
    # Process each file
    processed = 0
    errors = 0
    
    for file_path in sorted(dot_files):
        try:
            # Use a different seed for each file based on filename
            file_seed = args.seed + hash(file_path.name) % 10000
            process_single_file(file_path, args.probability, file_seed)
            processed += 1
            if processed % 100 == 0:
                print(f"Processed {processed}/{len(dot_files)} files...")
        except Exception as e:
            print(f"Error processing {file_path.name}: {e}")
            errors += 1
    
    print(f"\nCompleted!")
    print(f"Processed: {processed}")
    print(f"Errors: {errors}")
    print(f"Total files: {len(dot_files)}")
    
    return 0 if errors == 0 else 1

if __name__ == '__main__':
    exit(main())
