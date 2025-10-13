#!/usr/bin/env python3
"""
Simple Performance Visualizer - Clean Two-Line Comparison
Shows mean execution times for ontime vs temporis across vertex sizes
"""

import json
import matplotlib.pyplot as plt
import numpy as np

def create_clean_visualization():
    """Create a clean two-line performance comparison"""
    
    # Load the detailed results
    with open('/home/pete/temporis/benchmark/benchmark_results_detailed.json', 'r') as f:
        data = json.load(f)
    
    # Group results by vertex size
    size_groups = {}
    for result in data['results']:
        vertices = result['vertices']
        if vertices not in size_groups:
            size_groups[vertices] = {'ontime': [], 'temporis': []}
        
        # Only include successful runs
        if result['ontime']['success'] and result['temporis']['success']:
            size_groups[vertices]['ontime'].append(result['ontime']['runtime'])
            size_groups[vertices]['temporis'].append(result['temporis']['runtime'])
    
    # Calculate means for each size
    sizes = sorted(size_groups.keys())
    ontime_means = []
    temporis_means = []
    
    for size in sizes:
        if size_groups[size]['ontime'] and size_groups[size]['temporis']:
            ontime_means.append(np.mean(size_groups[size]['ontime']))
            temporis_means.append(np.mean(size_groups[size]['temporis']))
        else:
            # Skip sizes with no successful runs
            sizes.remove(size)
    
    # Create clean visualization
    plt.figure(figsize=(10, 6))
    
    # Plot the two lines
    plt.plot(sizes, ontime_means, 'o-', linewidth=3, markersize=8, 
             label='Ontime', color='#e74c3c', markerfacecolor='white', 
             markeredgewidth=2, markeredgecolor='#e74c3c')
    
    plt.plot(sizes, temporis_means, 's-', linewidth=3, markersize=8,
             label='Temporis', color='#3498db', markerfacecolor='white',
             markeredgewidth=2, markeredgecolor='#3498db')
    
    # Styling
    plt.xlabel('Number of Vertices', fontsize=14, fontweight='bold')
    plt.ylabel('Mean Execution Time (seconds)', fontsize=14, fontweight='bold')
    plt.title('Performance Comparison: Ontime vs Temporis\n'
              'Mean execution time across 15 games per vertex size', 
              fontsize=16, fontweight='bold', pad=20)
    
    # Legend
    plt.legend(fontsize=12, loc='upper left', framealpha=0.9)
    
    # Grid
    plt.grid(True, alpha=0.3, linestyle='--')
    
    # Use linear scale
    # (removed log scale for better linear comparison)
    
    # Adjust layout
    plt.tight_layout()
    
    # Save as PNG only
    plt.savefig('/home/pete/temporis/clean_performance_comparison.png', 
                dpi=300, bbox_inches='tight', facecolor='white')
    
    print("Clean visualization created:")
    print("  - clean_performance_comparison.png")

if __name__ == "__main__":
    create_clean_visualization()
