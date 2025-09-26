# Winning Region Randomization Report

## Summary
Successfully randomized winning regions in all 1000 temporal game files in `/home/pete/temporis/user_story_files/temporis_resources/converted_games/`.

## Process Details
- **Files processed**: 1000 .dot files
- **Errors encountered**: 0
- **Random seed used**: 42 (for reproducibility)
- **Target probability**: 30% per vertex

## Statistics
- **Total vertices across all games**: 9,204
- **Vertices marked as targets**: 2,766
- **Actual target percentage**: 30.05%

## What Was Changed
The randomization process modified the `target=1` attribute assignment in each game:

### Before Randomization
- Target vertices were originally assigned based on the conversion from ontime format
- Games had specific, deterministic winning regions

### After Randomization
- Each vertex in each game has a 30% probability of being marked as `target=1`
- The randomization uses a deterministic seed (42) combined with file-specific seeds for reproducibility
- All other game attributes (player assignments, constraints, edges) remain unchanged

## File Structure Preserved
- All DOT file syntax and formatting maintained
- Vertex names and player assignments unchanged
- Edge constraints and relationships preserved
- Only `target=1` attributes were randomized

## Games Affected
- Chain games: game_0001 through game_0200
- Branch games: game_0201 through game_0400  
- Cycle games: game_0401 through game_0600
- Complex games: game_0601 through game_1000

## Verification
Random sampling of processed files confirms:
- Proper DOT syntax maintained
- Target attributes correctly randomized
- Edge constraints preserved
- Game structure integrity maintained

The randomization is complete and all games now have randomized winning regions suitable for testing and benchmarking purposes.
