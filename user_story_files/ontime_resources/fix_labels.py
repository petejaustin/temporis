#!/usr/bin/env python3
"""
Script to fix temporal game files so that label names match node names.
This script modifies all .tg files in the ontime generated_games directory
by changing label["..."] to match the node name.
"""

import os
import re
import argparse
from pathlib import Path

def process_single_file(file_path, dry_run=False):
    """Process a single .tg file to make labels match node names."""
    
    with open(file_path, 'r') as f:
        content = f.read()
    
    lines = content.split('\n')
    new_lines = []
    changes_made = 0
    
    for line in lines:
        # Look for node declarations with labels
        node_match = re.match(r'^node\s+(\w+):\s*label\["([^"]+)"\],\s*(.*)$', line)
        
        if node_match:
            node_name = node_match.group(1)
            current_label = node_match.group(2)
            rest_of_line = node_match.group(3)
            
            # Check if label needs to be changed
            if current_label != node_name:
                new_line = f'node {node_name}: label["{node_name}"], {rest_of_line}'
                new_lines.append(new_line)
                changes_made += 1
                if not dry_run:
                    print(f"  {node_name}: '{current_label}' -> '{node_name}'")
            else:
                new_lines.append(line)
        else:
            new_lines.append(line)
    
    if changes_made > 0 and not dry_run:
        # Write back to file
        with open(file_path, 'w') as f:
            f.write('\n'.join(new_lines))
    
    return changes_made

def main():
    parser = argparse.ArgumentParser(description='Fix temporal game files so label names match node names')
    parser.add_argument('directory', nargs='?', 
                       default='/home/pete/temporis/user_story_files/ontime_resources/generated_games',
                       help='Directory containing .tg game files')
    parser.add_argument('--dry-run', action='store_true',
                       help='Show what would be done without making changes')
    
    args = parser.parse_args()
    
    directory = Path(args.directory)
    if not directory.exists():
        print(f"Error: Directory {directory} does not exist")
        return 1
    
    # Find all .tg files
    tg_files = list(directory.glob('*.tg'))
    if not tg_files:
        print(f"No .tg files found in {directory}")
        return 1
    
    print(f"Found {len(tg_files)} .tg files")
    
    if args.dry_run:
        print("DRY RUN - no files will be modified")
    
    # Process each file
    processed = 0
    total_changes = 0
    errors = 0
    
    for file_path in sorted(tg_files):
        try:
            if not args.dry_run:
                print(f"\nProcessing {file_path.name}...")
            
            changes = process_single_file(file_path, args.dry_run)
            
            if changes > 0:
                total_changes += changes
                if args.dry_run:
                    print(f"{file_path.name}: {changes} changes needed")
                else:
                    print(f"  Made {changes} changes")
            elif not args.dry_run:
                print(f"  No changes needed")
            
            processed += 1
            
            if processed % 100 == 0:
                print(f"Processed {processed}/{len(tg_files)} files...")
                
        except Exception as e:
            print(f"Error processing {file_path.name}: {e}")
            errors += 1
    
    print(f"\nCompleted!")
    print(f"Files processed: {processed}")
    print(f"Total label changes: {total_changes}")
    print(f"Errors: {errors}")
    
    return 0 if errors == 0 else 1

if __name__ == '__main__':
    exit(main())
