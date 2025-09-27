# TEMPORIS Resources Directory

This directory contains Python scripts and utilities for working with TEMPORIS temporal game solver. The workflow processes temporal games through conversion, randomization, and analysis phases.

## 📁 Directory Structure

```
temporis_resources/
├── convert_ontime_to_temporis.py    # Convert .tg files to .dot format
├── randomize_winning_regions.py     # Randomize target assignments
├── extract_targets.py               # Extract target vertex information
├── test_converted_games.sh          # Validate converted games
└── converted_games/                 # Output directory (1000 .dot files)
```

## 🔄 Complete Workflow

The typical workflow follows this sequence:

1. **Convert** ontime .tg files → TEMPORIS .dot format
2. **Randomize** winning regions (target assignments)  
3. **Extract** target vertices for analysis
4. **Test** converted games for validity

---

## 📜 Script Documentation

### 🔄 convert_ontime_to_temporis.py

**Purpose**: Converts ontime temporal graph (.tg) files to TEMPORIS DOT format (.dot)

**Usage**:
```bash
python3 convert_ontime_to_temporis.py [options]
```

**Options**:
- `--input-dir PATH`: Directory containing .tg files (default: `../ontime_resources/generated_games/`)
- `--output-dir PATH`: Directory for .dot files (default: `./converted_games/`)
- `--verbose`: Enable detailed logging

**What it does**:
- Reads ontime .tg format files with temporal conditions
- Converts to DOT format compatible with TEMPORIS solver
- Handles temporal constraints and player assignments
- Preserves game structure and timing semantics
- Creates 1000 converted .dot files

**Example**:
```bash
# Convert all games with default paths
python3 convert_ontime_to_temporis.py --verbose

# Custom input/output directories
python3 convert_ontime_to_temporis.py \
    --input-dir /path/to/tg/files \
    --output-dir /path/to/dot/files
```

**Output**: Creates `.dot` files in `converted_games/` directory

---

### 🎲 randomize_winning_regions.py

**Purpose**: Randomly reassigns target attributes (winning regions) across game vertices

**Usage**:
```bash
python3 randomize_winning_regions.py [options]
```

**Options**:
- `--input-dir PATH`: Directory with .dot files (default: `./converted_games/`)
- `--seed NUMBER`: Random seed for reproducibility (default: 42)
- `--target-prob FLOAT`: Probability of vertex being target (default: 0.3)
- `--dry-run`: Show what would be changed without modifying files

**What it does**:
- Scans all .dot files in target directory
- Identifies vertices with `target` attributes  
- Randomly reassigns `target=1` to approximately 30% of vertices
- Uses consistent random seed for reproducible results
- Preserves all other vertex/edge properties

**Example**:
```bash
# Default randomization with seed=42
python3 randomize_winning_regions.py

# Custom probability and seed
python3 randomize_winning_regions.py --target-prob 0.25 --seed 123

# Preview changes without modifying files
python3 randomize_winning_regions.py --dry-run
```

**Output**: 
- Modifies .dot files in-place
- Displays statistics summary in terminal

---

### 📊 extract_targets.py

**Purpose**: Extracts target vertex information from all converted games

**Usage**:
```bash
python3 extract_targets.py [options]
```

**Options**:
- `--input-dir PATH`: Directory with .dot files (default: `./converted_games/`)
- `--output-file PATH`: Output file for targets (default: `../ontime_resources/target_vertices.txt`)

**What it does**:
- Scans all .dot files for vertices with `target=1`
- Extracts target vertex names for each game
- Creates formatted output compatible with ontime solver
- Generates summary statistics

**Example**:
```bash
# Extract targets to default location
python3 extract_targets.py

# Custom output file
python3 extract_targets.py --output-file /path/to/targets.txt
```

**Output**:
- Creates `target_vertices.txt` with one line per game
- Displays extraction statistics in terminal

---

### 🧪 test_converted_games.sh

**Purpose**: Validates converted games using TEMPORIS solver

**Usage**:
```bash
./test_converted_games.sh [number_of_games]
```

**Arguments**:
- `number_of_games`: How many games to test (default: 10)

**What it does**:
- Tests random sample of converted .dot files
- Runs TEMPORIS solver on each game
- Reports success/failure rates
- Identifies problematic games

**Example**:
```bash
# Test 10 random games
./test_converted_games.sh

# Test 50 games  
./test_converted_games.sh 50
```

---

## 🚀 Quick Start Guide

### Complete Conversion Workflow

```bash
cd temporis_resources/

# 1. Convert ontime games to TEMPORIS format
python3 convert_ontime_to_temporis.py --verbose

# 2. Randomize winning regions with reproducible seed  
python3 randomize_winning_regions.py --seed 42

# 3. Extract target information for batch processing
python3 extract_targets.py

# 4. Validate some converted games
./test_converted_games.sh 20

# 5. Check results
ls -la converted_games/ | wc -l  # Should show ~1000 files
head target_vertices.txt         # Check target format
```

### Workflow Verification

```bash
# Check conversion succeeded
echo "Converted games: $(ls converted_games/*.dot | wc -l)"

# Verify randomization completed successfully
echo "Randomization completed - check converted_games directory"

# Check target extraction
wc -l target_vertices.txt  # Should be 1000 lines
```

---

## 📋 Dependencies

**Required**:
- Python 3.7+
- Standard libraries: `os`, `re`, `random`, `argparse`, `pathlib`

**For testing**:
- TEMPORIS solver built in `../../build/temporis`
- Bash shell for test scripts

---

## 📄 Output Files

| File | Purpose | Format |
|------|---------|---------|
| `converted_games/*.dot` | TEMPORIS-compatible games | DOT graph format |
| `target_vertices.txt` | Target vertex lists | One line per game |

---

## ⚠️ Important Notes

- **File Dependencies**: Scripts expect specific directory structure relative to `temporis_resources/`
- **Reproducibility**: Use consistent `--seed` values for reproducible randomization
- **Validation**: Always run `test_converted_games.sh` after conversion
- **Backup**: Original ontime .tg files remain unchanged in `ontime_resources/`

---

## 🔧 Troubleshooting

**Common Issues**:

1. **Missing input files**: Ensure ontime games are generated first
2. **Permission errors**: Make sure scripts are executable (`chmod +x`)  
3. **Path problems**: Run scripts from `temporis_resources/` directory
4. **TEMPORIS not found**: Build TEMPORIS solver first (`cmake --build ../../build`)

**Getting Help**:
```bash
python3 convert_ontime_to_temporis.py --help
python3 randomize_winning_regions.py --help  
python3 extract_targets.py --help
```
