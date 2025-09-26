#!/bin/bash
# Test script for converted temporis games
# This script demonstrates how to run the temporis solver on the converted games

echo "Testing converted temporal graph games with temporis solver"
echo "=========================================================="

# Check if temporis is built
if [ ! -f "build/temporis" ]; then
    echo "❌ Temporis not found. Building..."
    mkdir -p build && cd build && cmake .. && make -j$(nproc)
    cd ..
    if [ ! -f "build/temporis" ]; then
        echo "❌ Failed to build temporis"
        exit 1
    fi
fi

echo "✅ Temporis solver ready"
echo ""

# Test a few representative games
echo "Testing sample converted games:"
echo "------------------------------"

# Test 1: Simple chain
echo "🔍 Testing converted chain game:"
echo "Command: ./build/temporis converted_games/game_0001_chain_3.dot"
./build/temporis converted_games/game_0001_chain_3.dot
echo ""

# Test 2: Branching graph
echo "🔍 Testing converted branching game:"
echo "Command: ./build/temporis converted_games/game_0201_branch_3_3.dot"
./build/temporis converted_games/game_0201_branch_3_3.dot
echo ""

# Test 3: Cycle graph
echo "🔍 Testing converted cycle game:"
echo "Command: ./build/temporis converted_games/game_0401_cycle_4.dot"
./build/temporis converted_games/game_0401_cycle_4.dot
echo ""

# Test 4: Complex graph
echo "🔍 Testing converted complex game:"
echo "Command: ./build/temporis converted_games/game_0601_complex_10.dot"
./build/temporis converted_games/game_0601_complex_10.dot
echo ""

echo "Sample tests completed!"
echo ""
echo "💡 Usage examples:"
echo "  ./build/temporis converted_games/game_0001_chain_3.dot"
echo "  ./build/temporis converted_games/game_0201_branch_3_3.dot"
echo "  ./build/temporis converted_games/game_0401_cycle_4.dot"
echo "  ./build/temporis converted_games/game_0601_complex_10.dot"
echo ""
echo "📊 Converted game statistics:"
echo "  - Total games: 1000"
echo "  - Chain graphs: games 1-200 (linear structures)"
echo "  - Branching graphs: games 201-400 (tree-like structures)"
echo "  - Cycle graphs: games 401-600 (circular structures)"
echo "  - Complex graphs: games 601-1000 (dense connectivity)"
echo ""
echo "🔄 Format conversion details:"
echo "  - ontime .tg format → temporis .dot format"
echo "  - Temporal conditions: (= (mod t 3) 0) → time % 3 == 0"
echo "  - Explicit times: (0,3,7) → time == 0 || time == 3 || time == 7"
echo "  - Boolean logic: (or ...) → (...) || (...)"
echo "  - Negations: (not ...) → !(...)"
