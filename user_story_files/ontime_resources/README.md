# ONTIME Resources Directory

This directory contains Python scripts and utilities for working with the ONTIME punctual reachability solver. The workflow generates temporal games, processes them with ONTIME, and collects comprehensive results.

## 📁 Directory Structure

```
ontime_resources/
├── generate_games.py              # Generate 1000 temporal games  
├── fix_labels.py                 # Fix label/name mismatches
├── validate_games.py             # Validate generated games
├── enhanced_batch_processor.py   # Advanced batch processing with CSV output
├── run_ontime_batch.py          # Orchestrate batch processing runs
├── test_games.sh                # Shell script for game validation
├── generated_games/             # Output directory (1000 .tg files)
├── target_vertices.txt          # Target vertex lists (from temporis_resources)
└── ontime_all_results_t10000.csv # Complete results dataset
```

## 🔄 Complete Workflow

The typical workflow follows this sequence:

1. **Generate** 1000 temporal games in .tg format
2. **Fix** label/name inconsistencies  
3. **Validate** generated games for correctness
4. **Process** all games through ONTIME solver with batch automation
5. **Collect** comprehensive results in CSV format

---

## 📜 Script Documentation

### 🎮 generate_games.py

**Purpose**: Generate 1000 diverse temporal graph games for ONTIME solver testing

**Usage**:
```bash
python3 generate_games.py [options]
```

**Options**:
- `--output-dir PATH`: Directory for .tg files (default: `./generated_games/`)
- `--count NUMBER`: Number of games to generate (default: 1000)
- `--seed NUMBER`: Random seed for reproducibility (default: 42)
- `--verbose`: Enable detailed logging

**What it does**:
- Creates diverse temporal graph structures:
  - Simple chains and cycles (games 1-200)
  - Multi-path graphs (games 201-500)  
  - Complex temporal conditions (games 501-800)
  - Explicit time-based availability (games 801-1000)
- Uses modular arithmetic for temporal constraints
- Assigns random player ownership and target vertices
- Generates games with varying complexity and timing patterns

**Game Types Generated**:
- **Chain graphs**: Linear sequences with temporal delays
- **Cycle graphs**: Circular structures with timing constraints  
- **Multi-path**: Multiple routes with different temporal requirements
- **Complex**: Advanced timing patterns using modular conditions

**Example**:
```bash
# Generate 1000 games with default settings
python3 generate_games.py --verbose

# Generate 500 games with custom seed
python3 generate_games.py --count 500 --seed 123

# Custom output directory
python3 generate_games.py --output-dir /path/to/games/
```

**Output**: Creates `.tg` files named `game_XXXX_TYPE_SIZE.tg`

---

### 🔧 fix_labels.py

**Purpose**: Fix mismatches between vertex labels and node names in .tg files

**Usage**:
```bash
python3 fix_labels.py [options]
```

**Options**:
- `--input-dir PATH`: Directory with .tg files (default: `./generated_games/`)
- `--dry-run`: Preview changes without modifying files
- `--verbose`: Show detailed progress

**What it does**:
- Scans all .tg files for label/name inconsistencies
- Updates `label="name"` to match actual node names
- Preserves all other vertex properties
- Creates backup information before modifications
- Maintains file structure and formatting

**Example**:
```bash
# Fix all label issues
python3 fix_labels.py --verbose

# Preview what would be changed
python3 fix_labels.py --dry-run

# Process specific directory
python3 fix_labels.py --input-dir /path/to/games/
```

**Output**:
- Modifies .tg files in-place
- Displays fix statistics in terminal

---

### ✅ validate_games.py

**Purpose**: Validate generated temporal games for correctness and consistency

**Usage**:
```bash
python3 validate_games.py [options]
```

**Options**:
- `--input-dir PATH`: Directory with .tg files (default: `./generated_games/`)
- `--sample-size NUMBER`: Number of games to validate (default: all)
- `--verbose`: Show detailed validation results

**What it does**:
- Checks .tg file syntax and structure
- Validates temporal constraint formats
- Ensures player assignments are consistent  
- Verifies edge definitions and connectivity
- Reports games with potential issues

**Validation Checks**:
- File format correctness
- Node/edge syntax validation
- Temporal constraint parsing
- Player assignment verification
- Target vertex validation

**Example**:
```bash
# Validate all games
python3 validate_games.py --verbose

# Quick validation of 50 random games
python3 validate_games.py --sample-size 50

# Validate specific directory
python3 validate_games.py --input-dir /path/to/games/
```

**Output**: Detailed validation report with any issues found

---

### 🚀 enhanced_batch_processor.py

**Purpose**: Advanced batch processing engine for ONTIME solver with comprehensive CSV output

**Usage**:
```bash
python3 enhanced_batch_processor.py [options]
```

**Options**:
- `--start-game NUMBER`: Starting game number (default: 1)
- `--end-game NUMBER`: Ending game number (default: 100)  
- `--time-bound NUMBER`: Target time for solver (default: 10000)
- `--ontime-path PATH`: Path to ontime executable
- `--games-dir PATH`: Directory with .tg files
- `--targets-file PATH`: File with target vertex lists
- `--output-csv PATH`: CSV file for results
- `--timeout NUMBER`: Solver timeout in seconds (default: 30)

**What it does**:
- Processes specified range of games through ONTIME solver
- Extracts target vertices for each game
- Runs ONTIME with proper command-line arguments  
- Parses solver output to extract winning regions at t=0 and t=target_time
- Records solve times and success/failure status
- Generates detailed CSV output with all results

**CSV Output Columns**:
- `game_number`: Sequential game identifier
- `game_name`: Original .tg filename
- `target_vertices`: List of target vertex names
- `success`: Boolean solver success status
- `solve_time_seconds`: Time taken by solver
- `winning_region_t0`: Winning vertices at time 0
- `winning_region_t10000`: Winning vertices at target time

**Example**:
```bash
# Process games 1-100 with default settings
python3 enhanced_batch_processor.py --end-game 100

# Process specific range with custom timeout
python3 enhanced_batch_processor.py \
    --start-game 501 --end-game 600 \
    --timeout 60 --time-bound 5000

# Custom paths and output
python3 enhanced_batch_processor.py \
    --ontime-path /path/to/ontime \
    --games-dir /path/to/games \
    --output-csv results_custom.csv
```

**Output**: CSV file with comprehensive results for processed games

---

### 📊 run_ontime_batch.py

**Purpose**: Orchestrate multiple batch processing runs to handle all 1000 games

**Usage**:
```bash
python3 run_ontime_batch.py [options]
```

**Options**:
- `--batch-size NUMBER`: Games per batch (default: 100)
- `--total-games NUMBER`: Total number of games (default: 1000)
- `--time-bound NUMBER`: Target time for solver (default: 10000)
- `--base-output PATH`: Base name for output files

**What it does**:
- Divides 1000 games into manageable batches (typically 10 batches of 100)
- Runs `enhanced_batch_processor.py` for each batch
- Collects intermediate CSV files  
- Combines results into final comprehensive dataset
- Handles batch failures and retries
- Provides progress tracking and time estimates

**Batch Management**:
- Creates separate CSV files for each batch
- Allows resuming from failed batches
- Provides detailed progress logging
- Combines all results into master CSV file

**Example**:
```bash
# Process all 1000 games in batches of 100
python3 run_ontime_batch.py

# Custom batch size and total
python3 run_ontime_batch.py --batch-size 50 --total-games 500

# Different time bound
python3 run_ontime_batch.py --time-bound 5000
```

**Output**: 
- Individual batch CSV files (`batch_1.csv`, `batch_2.csv`, ...)
- Combined final results (`ontime_all_results_t10000.csv`)

---

### 🧪 test_games.sh

**Purpose**: Shell script for quick validation of generated games

**Usage**:
```bash
./test_games.sh [number_of_games]
```

**Arguments**:
- `number_of_games`: How many games to test (default: 20)

**What it does**:
- Randomly samples generated .tg files
- Attempts to run ONTIME solver on each
- Reports basic success/failure statistics
- Identifies games with parsing or solver issues

**Example**:
```bash
# Test 20 random games
./test_games.sh

# Test 100 games
./test_games.sh 100
```

---

## 🚀 Quick Start Guide

### Complete ONTIME Workflow

```bash
cd ontime_resources/

# 1. Generate 1000 temporal games
python3 generate_games.py --verbose

# 2. Fix any label/name mismatches  
python3 fix_labels.py --verbose

# 3. Validate generated games
python3 validate_games.py --sample-size 50

# 4. Test some games with ONTIME
./test_games.sh 20

# 5. Process all games through ONTIME (this takes time!)
python3 run_ontime_batch.py

# 6. Check final results
head ontime_all_results_t10000.csv
wc -l ontime_all_results_t10000.csv  # Should be 1001 (header + 1000 games)
```

### Individual Batch Processing

```bash
# Process specific game ranges manually
python3 enhanced_batch_processor.py --start-game 1 --end-game 100
python3 enhanced_batch_processor.py --start-game 101 --end-game 200
# ... etc
```

### Workflow Verification

```bash
# Check game generation
echo "Generated games: $(ls generated_games/*.tg | wc -l)"

# Verify ONTIME is available
which ontime  # Should show path to ontime executable

# Check target vertices file (created by temporis workflow)
head target_vertices.txt
wc -l target_vertices.txt  # Should be 1000 lines
```

---

## 📋 Dependencies

**Required**:
- Python 3.7+
- Standard libraries: `os`, `subprocess`, `csv`, `json`, `time`, `re`, `pathlib`
- ONTIME solver executable in PATH or specified location

**External Dependencies**:
- `target_vertices.txt` (generated by temporis workflow)
- ONTIME solver built and available

**System Requirements**:
- Sufficient disk space for 1000 .tg files (~50MB)
- Memory for batch processing (depends on game complexity)
- Time: Full 1000-game processing can take 30-60 minutes

---

## 📄 Output Files

| File | Purpose | Format |
|------|---------|---------|
| `generated_games/*.tg` | ONTIME temporal games | .tg format |
| `ontime_all_results_t10000.csv` | Complete solver results | CSV with all metrics |
| `batch_*.csv` | Individual batch results | CSV format |  
| `target_vertices.txt` | Target vertex lists | One line per game |

---

## ⚠️ Important Notes

- **Processing Time**: Full 1000-game batch processing takes significant time
- **Memory Usage**: Monitor system resources during large batch runs
- **Dependencies**: Requires `target_vertices.txt` from temporis workflow
- **ONTIME Location**: Ensure ONTIME solver is built and accessible
- **Sequential Numbering**: Final CSV uses sequential game numbers (1-1000)

---

## 🔧 Troubleshooting

**Common Issues**:

1. **ONTIME not found**: 
   ```bash
   # Build ONTIME solver first
   cd /path/to/ontime && cargo build --release
   ```

2. **Missing target_vertices.txt**: Run temporis workflow first
   ```bash
   cd ../temporis_resources/
   python3 extract_targets.py
   ```

3. **Batch processing failures**: Check individual batch logs
   ```bash
   # Resume from specific batch
   python3 enhanced_batch_processor.py --start-game 501 --end-game 600
   ```

4. **Memory issues**: Reduce batch size
   ```bash
   python3 run_ontime_batch.py --batch-size 50
   ```

**Getting Help**:
```bash
python3 generate_games.py --help
python3 enhanced_batch_processor.py --help
python3 run_ontime_batch.py --help
```

---

## 📈 Performance Tips

- **Parallel Processing**: Consider splitting batches across multiple terminals
- **Resource Monitoring**: Use `htop` or similar to monitor system load
- **Incremental Processing**: Process games in smaller batches if memory limited
- **Result Validation**: Regularly check intermediate CSV files during processing
