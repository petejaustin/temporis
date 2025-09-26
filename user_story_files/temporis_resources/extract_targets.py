#!/usr/bin/env python3
"""
Script to extract target vertex labels from converted temporal games.
Creates a text file with comma-separated target vertex names, one line per game.
"""

import os
import re
import argparse
from pathlib import Path

def extract_target_vertices(file_path):
    """Extract target vertices from a single .dot file."""
    targets = []
    
    try:
        with open(file_path, 'r') as f:
            content = f.read()
        
        # Find all vertex declarations with target=1
        # Pattern matches: vertex_name [name="vertex_name", player=X, target=1];
        target_pattern = r'(\w+)\s*\[([^\]]*target=1[^\]]*)\];'
        matches = re.findall(target_pattern, content)
        
        for vertex_name, attributes in matches:
            targets.append(vertex_name)
    
    except Exception as e:
        print(f"Error processing {file_path.name}: {e}")
    
    return targets

def main():
    parser = argparse.ArgumentParser(description='Extract target vertices from converted temporal games')
    parser.add_argument('input_dir', nargs='?', 
                       default='/home/pete/temporis/user_story_files/temporis_resources/converted_games',
                       help='Directory containing .dot game files')
    parser.add_argument('output_file', nargs='?',
                       default='/home/pete/temporis/user_story_files/temporis_resources/target_vertices.txt',
                       help='Output file for target vertices')
    
    args = parser.parse_args()
    
    input_dir = Path(args.input_dir)
    output_file = Path(args.output_file)
    
    if not input_dir.exists():
        print(f"Error: Directory {input_dir} does not exist")
        return 1
    
    # Find all .dot files
    dot_files = list(input_dir.glob('*.dot'))
    if not dot_files:
        print(f"No .dot files found in {input_dir}")
        return 1
    
    print(f"Found {len(dot_files)} .dot files")
    print(f"Output file: {output_file}")
    
    # Process each file and collect results
    results = []
    processed = 0
    errors = 0
    
    for file_path in sorted(dot_files):
        try:
            targets = extract_target_vertices(file_path)
            # Join targets with commas, or empty string if no targets
            target_line = ','.join(targets) if targets else ''
            results.append(f"{target_line}")
            
            processed += 1
            if processed % 100 == 0:
                print(f"Processed {processed}/{len(dot_files)} files...")
                
        except Exception as e:
            print(f"Error processing {file_path.name}: {e}")
            errors += 1
            results.append('')  # Add empty line for failed files
    
    # Write results to output file
    try:
        with open(output_file, 'w') as f:
            for line in results:
                f.write(line + '\n')
        
        print(f"\nCompleted!")
        print(f"Files processed: {processed}")
        print(f"Errors: {errors}")
        print(f"Results written to: {output_file}")
        
        # Show some statistics
        non_empty_lines = sum(1 for line in results if line.strip())
        total_targets = sum(len(line.split(',')) for line in results if line.strip())
        
        print(f"Games with targets: {non_empty_lines}/{len(results)}")
        print(f"Total target vertices: {total_targets}")
        
    except Exception as e:
        print(f"Error writing output file: {e}")
        return 1
    
    return 0 if errors == 0 else 1

if __name__ == '__main__':
    exit(main())
