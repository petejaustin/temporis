#!/bin/bash
# Sample test script for the generated temporal graph games
# This script demonstrates how to run the ontime solver on the generated games

echo "Testing generated temporal graph games with ontime solver"
echo "========================================================="

# Check if cargo is available
if ! command -v cargo &> /dev/null; then
    echo "❌ Cargo/Rust not found. Please install Rust first:"
    echo "   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh"
    echo "   source ~/.cargo/env"
    exit 1
fi

# Build the ontime solver
echo "Building ontime solver..."
cargo build --release

if [ $? -ne 0 ]; then
    echo "❌ Failed to build ontime solver"
    exit 1
fi

echo "✅ Build successful"
echo ""

# Test a few representative games
echo "Testing sample games:"
echo "--------------------"

# Test 1: Simple chain
echo "🔍 Testing chain game:"
echo "Command: cargo run -- generated_games/game_0001_chain_3.tg \"s0,s2\" 10"
cargo run -- generated_games/game_0001_chain_3.tg "s0,s2" 10
echo ""

# Test 2: Branching graph
echo "🔍 Testing branching game:"
echo "Command: cargo run -- generated_games/game_0201_branch_3_3.tg \"s4,s12\" 15"
cargo run -- generated_games/game_0201_branch_3_3.tg "s4,s12" 15
echo ""

# Test 3: Cycle graph
echo "🔍 Testing cycle game:"
echo "Command: cargo run -- generated_games/game_0401_cycle_4.tg \"s0,s3\" 8"
cargo run -- generated_games/game_0401_cycle_4.tg "s0,s3" 8
echo ""

# Test 4: Complex graph
echo "🔍 Testing complex game:"
echo "Command: cargo run -- generated_games/game_0601_complex_10.tg \"s9\" 20"
cargo run -- generated_games/game_0601_complex_10.tg "s9" 20
echo ""

echo "Sample tests completed!"
echo ""
echo "💡 Usage examples:"
echo "  - Simple reachability: cargo run -- generated_games/game_XXXX.tg \"target_node\" time"
echo "  - Multiple targets: cargo run -- generated_games/game_XXXX.tg \"node1,node2,node3\" time"
echo "  - Try different time horizons to see how temporal conditions affect reachability"
echo ""
echo "📊 Game statistics:"
echo "  - Total games: 1000"
echo "  - Chain graphs: games 1-200 (linear structures)"
echo "  - Branching graphs: games 201-400 (tree-like structures)"
echo "  - Cycle graphs: games 401-600 (circular structures)"
echo "  - Complex graphs: games 601-1000 (dense connectivity)"
