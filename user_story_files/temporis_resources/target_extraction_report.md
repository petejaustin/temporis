# Target Vertices Extraction Report

## Summary
Successfully extracted target vertex labels from all 1000 converted temporal game files and created a comma-separated output file.

## Output File Details
- **Location**: `/home/pete/temporis/user_story_files/temporis_resources/target_vertices.txt`
- **Format**: One line per game, comma-separated target vertex names
- **Total lines**: 1,000 (one for each game file)

## Statistics
- **Games processed**: 1,000/1,000 (100% success rate)
- **Games with target vertices**: 887/1,000 (88.7%)
- **Games without targets**: 113/1,000 (11.3% - represented as empty lines)
- **Total target vertices**: 2,766 across all games

## File Format
Each line corresponds to one game file (in alphabetical order by filename):
- **Line with targets**: `s0,s1,s4` (comma-separated vertex names)
- **Line without targets**: `` (empty line)

## Sample Output
```
s0,s1                    # Game 1: 2 target vertices
s4,s5,s6,s11            # Game 2: 4 target vertices  
s3,s4,s9                # Game 3: 3 target vertices
s4                      # Game 4: 1 target vertex
                        # Game 5: no target vertices
s0,s4,s7                # Game 6: 3 target vertices
```

## Extraction Method
- Parsed each `.dot` file for vertices with `target=1` attribute
- Extracted vertex names (s0, s1, s2, etc.) from matching declarations
- Joined multiple targets per game with commas
- Maintained game order by processing files alphabetically

## Verification
- All 1000 games processed successfully with 0 errors
- Target count (2,766) matches the randomized winning regions created earlier
- File format confirmed: exactly 1000 lines, properly comma-separated values
- Empty lines correctly represent games without target vertices

The target vertices file is ready for use and contains all target vertex labels from the converted temporal games in the requested format.
