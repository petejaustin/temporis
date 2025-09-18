#include "temporal_reachability_solver.hpp"
#include <boost/graph/graph_traits.hpp>
#include <iostream>
#include <algorithm>

TemporalReachabilitySolver::TemporalReachabilitySolver(PresburgerTemporalGameManager& manager,
                                                      std::shared_ptr<ReachabilityObjective> objective,
                                                      int max_time)
    : manager_(manager), objective_(objective), max_time_(max_time) {
}

ReachabilityGameSolution TemporalReachabilitySolver::solve(PresburgerTemporalVertex initial_vertex, int initial_time) {
    memo_.clear();
    GameState initial_state{initial_vertex, initial_time};
    std::set<GameState> visited;
    
    ReachabilityGameSolution::Winner winner = solve_recursive(initial_state, visited);
    
    return build_solution(initial_vertex, initial_time);
}

ReachabilityGameSolution::Winner TemporalReachabilitySolver::solve_recursive(const GameState& state, std::set<GameState>& visited) {
    // Check memoization
    if (memo_.find(state) != memo_.end()) {
        return memo_[state];
    }
    
    // Check for cycles
    if (visited.find(state) != visited.end()) {
        // In a cycle - outcome depends on objective type
        memo_[state] = ReachabilityGameSolution::Winner::PLAYER_1; // Conservative: assume Player 1 wins
        return memo_[state];
    }
    
    // Check if terminal state
    ReachabilityGameSolution::Winner terminal_winner;
    if (is_terminal_state(state, terminal_winner)) {
        memo_[state] = terminal_winner;
        return terminal_winner;
    }
    
    // Check time bound
    if (state.time >= max_time_) {
        memo_[state] = ReachabilityGameSolution::Winner::PLAYER_1; // Time expired, Player 1 wins
        return memo_[state];
    }
    
    visited.insert(state);
    
    // Get current player
    int current_player = manager_.graph()[state.vertex].player;
    std::vector<PresburgerTemporalVertex> moves = get_available_moves(state);
    
    if (moves.empty()) {
        // No moves available - game ends
        memo_[state] = ReachabilityGameSolution::Winner::PLAYER_1;
        visited.erase(state);
        return memo_[state];
    }
    
    ReachabilityGameSolution::Winner result;
    
    if (current_player == 0) {
        // Player 0 maximizes (tries to win)
        result = ReachabilityGameSolution::Winner::PLAYER_1; // Start pessimistic
        for (auto next_vertex : moves) {
            GameState next_state{next_vertex, state.time + 1};
            ReachabilityGameSolution::Winner next_result = solve_recursive(next_state, visited);
            if (next_result == ReachabilityGameSolution::Winner::PLAYER_0) {
                result = ReachabilityGameSolution::Winner::PLAYER_0;
                break; // Player 0 found a winning move
            }
        }
    } else {
        // Player 1 minimizes (tries to prevent Player 0 from winning)
        result = ReachabilityGameSolution::Winner::PLAYER_0; // Start optimistic
        for (auto next_vertex : moves) {
            GameState next_state{next_vertex, state.time + 1};
            ReachabilityGameSolution::Winner next_result = solve_recursive(next_state, visited);
            if (next_result == ReachabilityGameSolution::Winner::PLAYER_1) {
                result = ReachabilityGameSolution::Winner::PLAYER_1;
                break; // Player 1 found a move to prevent Player 0 from winning
            }
        }
    }
    
    memo_[state] = result;
    visited.erase(state);
    return result;
}

bool TemporalReachabilitySolver::is_terminal_state(const GameState& state, ReachabilityGameSolution::Winner& winner) {
    if (objective_->is_satisfied(state.vertex, state.time)) {
        winner = ReachabilityGameSolution::Winner::PLAYER_0;
        return true;
    }
    
    if (objective_->has_failed(state.vertex, state.time)) {
        winner = ReachabilityGameSolution::Winner::PLAYER_1;
        return true;
    }
    
    return false;
}

std::vector<PresburgerTemporalVertex> TemporalReachabilitySolver::get_available_moves(const GameState& state) {
    std::vector<PresburgerTemporalVertex> moves;
    
    auto [edges_begin, edges_end] = boost::out_edges(state.vertex, manager_.graph());
    for (auto edge_it = edges_begin; edge_it != edges_end; ++edge_it) {
        if (manager_.is_edge_constraint_satisfied(*edge_it, state.time)) {
            PresburgerTemporalVertex target = boost::target(*edge_it, manager_.graph());
            moves.push_back(target);
        }
    }
    
    return moves;
}

bool TemporalReachabilitySolver::can_win_from_state(const GameState& state, int player) {
    if (memo_.find(state) == memo_.end()) {
        std::set<GameState> visited;
        solve_recursive(state, visited);
    }
    
    ReachabilityGameSolution::Winner winner = memo_[state];
    return (player == 0 && winner == ReachabilityGameSolution::Winner::PLAYER_0) ||
           (player == 1 && winner == ReachabilityGameSolution::Winner::PLAYER_1);
}

ReachabilityGameSolution TemporalReachabilitySolver::build_solution(PresburgerTemporalVertex initial_vertex, int initial_time) {
    ReachabilityGameSolution solution;
    GameState initial_state{initial_vertex, initial_time};
    
    if (memo_.find(initial_state) != memo_.end()) {
        solution.winner = memo_[initial_state];
    }
    
    // Copy winning regions from memo
    solution.winning_regions = memo_;
    
    // Build strategy (simplified - pick first winning move)
    for (const auto& [state, winner] : memo_) {
        int current_player = manager_.graph()[state.vertex].player;
        if ((current_player == 0 && winner == ReachabilityGameSolution::Winner::PLAYER_0) ||
            (current_player == 1 && winner == ReachabilityGameSolution::Winner::PLAYER_1)) {
            
            auto moves = get_available_moves(state);
            for (auto move : moves) {
                GameState next_state{move, state.time + 1};
                if (memo_.find(next_state) != memo_.end() && memo_[next_state] == winner) {
                    solution.strategy[state] = move;
                    break;
                }
            }
        }
    }
    
    // Generate explanation
    if (solution.winner == ReachabilityGameSolution::Winner::PLAYER_0) {
        solution.explanation = "Player 0 (minimizer) has a winning strategy for the reachability objective.";
    } else if (solution.winner == ReachabilityGameSolution::Winner::PLAYER_1) {
        solution.explanation = "Player 1 (maximizer) has a winning strategy to prevent the reachability objective.";
    } else {
        solution.explanation = "Winner could not be determined within the given time bound.";
    }
    
    return solution;
}

void TemporalReachabilitySolver::print_solution_analysis(const ReachabilityGameSolution& solution, 
                                                        const GameState& initial_state) {
    std::cout << "\n=== Temporal Reachability Game Solution ===\n";
    std::cout << "Objective: " << objective_->to_string() << "\n\n";
    
    std::cout << "Winner: ";
    switch (solution.winner) {
        case ReachabilityGameSolution::Winner::PLAYER_0:
            std::cout << "Player 0\n";
            break;
        case ReachabilityGameSolution::Winner::PLAYER_1:
            std::cout << "Player 1\n";
            break;
        case ReachabilityGameSolution::Winner::UNDETERMINED:
            std::cout << "Undetermined\n";
            break;
    }
    
    std::cout << "Explanation: " << solution.explanation << "\n\n";
    
    // Print sample strategy from initial state
    std::cout << "Sample strategy from initial state:\n";
    GameState current = initial_state;
    int steps = 0;
    const int max_strategy_steps = 10;
    
    while (steps < max_strategy_steps && solution.strategy.find(current) != solution.strategy.end()) {
        PresburgerTemporalVertex next_move = solution.strategy.at(current);
        std::cout << "Time " << current.time << ": " 
                  << manager_.graph()[current.vertex].name 
                  << " (Player " << manager_.graph()[current.vertex].player << ") -> "
                  << manager_.graph()[next_move].name << "\n";
        
        current = GameState{next_move, current.time + 1};
        steps++;
        
        // Check if reached target
        if (objective_->is_target(current.vertex)) {
            std::cout << "Time " << current.time << ": Reached target " 
                      << manager_.graph()[current.vertex].name << "\n";
            break;
        }
    }
    
    if (steps >= max_strategy_steps) {
        std::cout << "... (strategy continues)\n";
    }
}

std::pair<std::set<PresburgerTemporalVertex>, std::set<PresburgerTemporalVertex>> 
TemporalReachabilitySolver::compute_winning_regions(int initial_time) {
    std::set<PresburgerTemporalVertex> player0_winning;
    std::set<PresburgerTemporalVertex> player1_winning;
    
    // Clear memoization to ensure fresh computation
    memo_.clear();
    
    // Get all vertices in the game
    auto [vertices_begin, vertices_end] = boost::vertices(manager_.graph());
    
    for (auto vertex_it = vertices_begin; vertex_it != vertices_end; ++vertex_it) {
        PresburgerTemporalVertex vertex = *vertex_it;
        GameState state{vertex, initial_time};
        std::set<GameState> visited;
        
        // Check if this vertex is itself a target
        if (objective_->is_target(vertex)) {
            player0_winning.insert(vertex);
            continue;
        }
        
        ReachabilityGameSolution::Winner winner = solve_recursive(state, visited);
        
        if (winner == ReachabilityGameSolution::Winner::PLAYER_0) {
            player0_winning.insert(vertex);
        } else if (winner == ReachabilityGameSolution::Winner::PLAYER_1) {
            player1_winning.insert(vertex);
        }
    }
    
    return std::make_pair(player0_winning, player1_winning);
}

void TemporalReachabilitySolver::print_winning_regions_analysis(
    const std::set<PresburgerTemporalVertex>& player0_winning,
    const std::set<PresburgerTemporalVertex>& player1_winning) {
    
    std::cout << "\n=== Winning Regions Analysis ===\n";
    
    // Print target vertices for context
    std::cout << "Target vertices: ";
    for (const auto& target : objective_->get_targets()) {
        std::cout << manager_.graph()[target].name << " ";
    }
    std::cout << "\n\n";
    
    std::cout << "Winning regions:\n";
    
    // Player 0 winning region
    std::cout << "Player 0: ";
    if (player0_winning.empty()) {
        std::cout << "(none)";
    } else {
        bool first = true;
        for (const auto& vertex : player0_winning) {
            if (!first) std::cout << " ";
            std::cout << manager_.graph()[vertex].name;
            first = false;
        }
    }
    std::cout << "\n";
    
    // Player 1 winning region
    std::cout << "Player 1: ";
    if (player1_winning.empty()) {
        std::cout << "(none)";
    } else {
        bool first = true;
        for (const auto& vertex : player1_winning) {
            if (!first) std::cout << " ";
            std::cout << manager_.graph()[vertex].name;
            first = false;
        }
    }
    std::cout << "\n";
}
