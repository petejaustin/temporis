#include "ggg_temporal_solver.hpp"
#include <boost/graph/graph_traits.hpp>
#include <iostream>
#include <algorithm>

namespace ggg {
namespace solvers {

GGGTemporalReachabilitySolver::GGGTemporalReachabilitySolver(
    std::shared_ptr<graphs::GGGTemporalGameManager> manager,
    std::shared_ptr<graphs::GGGReachabilityObjective> objective,
    int max_time, bool verbose)
    : manager_(manager), objective_(objective), max_time_(max_time), verbose_(verbose) {
}

std::string GGGTemporalReachabilitySolver::get_name() const {
    return "Backwards Temporal Attractor Solver";
}

GGGTemporalReachabilitySolver::SolutionType GGGTemporalReachabilitySolver::solve(const GraphType& graph) {
    // Reset statistics for this solve
    stats_.reset();
    auto solve_start = std::chrono::high_resolution_clock::now();
    
    // Compute winning regions using backwards temporal attractor
    auto [player0_winning, player1_winning] = compute_winning_regions(0);
    
    // "Solved" means we successfully computed winning regions for all vertices
    SolutionType solution(true);
    
    // Set winning regions
    for (auto vertex : player0_winning) {
        solution.set_winning_player(vertex, 0);
    }
    for (auto vertex : player1_winning) {
        solution.set_winning_player(vertex, 1);
    }
    
    // Build basic strategies - for now, just pick any valid move for winning vertices
    // This could be enhanced to build optimal strategies from the attractor computation
    auto [vertex_begin, vertex_end] = boost::vertices(graph);
    for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
        Vertex vertex = *vertex_it;
        
        if (player0_winning.find(vertex) != player0_winning.end()) {
            // This vertex is winning for player 0, find a valid move at time 0
            auto moves = manager_->get_available_moves(vertex, 0);
            if (!moves.empty()) {
                // For now, just pick the first available move
                // TODO: Could be enhanced to pick moves that stay in winning region
                solution.set_strategy(vertex, moves[0]);
            }
        }
    }
    
    // Record total solve time
    auto solve_end = std::chrono::high_resolution_clock::now();
    stats_.total_solve_time = solve_end - solve_start;
    
    return solution;
}

GGGTemporalReachabilitySolver::SolutionType GGGTemporalReachabilitySolver::solve_from_state(Vertex initial_vertex, int initial_time) {
    // For backwards attractor, initial_time doesn't affect the computation
    // We always compute from max_time back to 0
    auto [player0_winning, player1_winning] = compute_winning_regions(0);
    
    SolutionType solution(true);
    
    // Check if the initial vertex is in the winning region for player 0
    if (player0_winning.find(initial_vertex) != player0_winning.end()) {
        solution.set_winning_player(initial_vertex, 0);
        
        // Find a strategy move if available
        auto moves = manager_->get_available_moves(initial_vertex, initial_time);
        if (!moves.empty()) {
            solution.set_strategy(initial_vertex, moves[0]);
        }
    } else {
        solution.set_winning_player(initial_vertex, 1);
    }
    
    return solution;
}

std::pair<std::set<GGGTemporalReachabilitySolver::Vertex>, std::set<GGGTemporalReachabilitySolver::Vertex>>
GGGTemporalReachabilitySolver::compute_winning_regions(int initial_time) {
    memo_.clear();
    
    // Time the graph traversal
    auto traversal_start = std::chrono::high_resolution_clock::now();
    
    // Backwards temporal attractor computation
    std::set<Vertex> current_attractor = compute_backwards_temporal_attractor();
    
    // Split vertices into winning regions based on final attractor
    std::set<Vertex> player0_winning = current_attractor;
    std::set<Vertex> player1_winning;
    
    auto [vertex_begin, vertex_end] = boost::vertices(*manager_->graph());
    for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
        Vertex vertex = *vertex_it;
        if (player0_winning.find(vertex) == player0_winning.end()) {
            player1_winning.insert(vertex);
        }
    }
    
    auto traversal_end = std::chrono::high_resolution_clock::now();
    stats_.graph_traversal_time += (traversal_end - traversal_start);
    
    if (verbose_) {
        std::cout << "Final attractor at time 0 has " << player0_winning.size() << " vertices: {";
        bool first = true;
        for (auto vertex : player0_winning) {
            if (!first) std::cout << ", ";
            std::cout << (*manager_->graph())[vertex].name;
            first = false;
        }
        std::cout << "}\n";
    }
    
    return {player0_winning, player1_winning};
}

int GGGTemporalReachabilitySolver::solve_recursive(const TemporalGameState& state, std::set<TemporalGameState>& visited) {
    // Track state exploration
    stats_.states_explored++;
    stats_.max_time_reached = std::max(stats_.max_time_reached, static_cast<size_t>(state.time));
    
    // Check memoization
    if (memo_.find(state) != memo_.end()) {
        stats_.cache_hits++;
        return memo_[state];
    }
    stats_.cache_misses++;
    
    // Check for cycles
    if (visited.find(state) != visited.end()) {
        memo_[state] = -1; // Conservative: assume Player 1 wins in cycles
        return memo_[state];
    }
    
    // Check if terminal state
    int terminal_winner;
    if (is_terminal_state(state, terminal_winner)) {
        memo_[state] = terminal_winner;
        return terminal_winner;
    }
    
    // Check time bound
    if (state.time >= max_time_) {
        stats_.states_pruned++;
        memo_[state] = -1; // Time expired, Player 1 wins
        return memo_[state];
    }
    
    visited.insert(state);
    
    // Get current player
    int current_player = (*manager_->graph())[state.vertex].player;
    std::vector<Vertex> moves = get_available_moves(state);
    
    if (moves.empty()) {
        // No moves available - game ends, Player 1 wins
        memo_[state] = -1;
        visited.erase(state);
        return memo_[state];
    }
    
    int result;
    
    if (current_player == 0) {
        // Player 0 maximizes (tries to win)
        result = -1; // Start pessimistic
        for (auto next_vertex : moves) {
            TemporalGameState next_state{next_vertex, state.time + 1};
            int next_result = solve_recursive(next_state, visited);
            if (next_result > 0) {
                result = 1;
                break; // Player 0 found a winning move
            }
        }
    } else {
        // Player 1 minimizes (tries to prevent Player 0 from winning)
        result = 1; // Start optimistic for Player 0
        for (auto next_vertex : moves) {
            TemporalGameState next_state{next_vertex, state.time + 1};
            int next_result = solve_recursive(next_state, visited);
            if (next_result < 0) {
                result = -1;
                break; // Player 1 found a move to prevent Player 0 from winning
            }
        }
    }
    
    memo_[state] = result;
    visited.erase(state);
    return result;
}

bool GGGTemporalReachabilitySolver::is_terminal_state(const TemporalGameState& state, int& winner) {
    if (objective_->is_satisfied(state.vertex, state.time)) {
        winner = 1; // Player 0 wins (reachability satisfied)
        return true;
    }
    
    if (objective_->has_failed(state.vertex, state.time)) {
        winner = -1; // Player 1 wins (reachability failed)
        return true;
    }
    
    return false;
}

std::vector<GGGTemporalReachabilitySolver::Vertex> GGGTemporalReachabilitySolver::get_available_moves(const TemporalGameState& state) {
    // Time the constraint evaluation
    auto constraint_start = std::chrono::high_resolution_clock::now();
    
    auto moves = manager_->get_available_moves(state.vertex, state.time);
    
    auto constraint_end = std::chrono::high_resolution_clock::now();
    stats_.constraint_eval_time += (constraint_end - constraint_start);
    
    // Track constraint evaluation statistics
    stats_.constraint_evaluations++;
    if (!moves.empty()) {
        stats_.constraint_passes++;
    } else {
        stats_.constraint_failures++;
    }
    
    return moves;
}

std::set<GGGTemporalReachabilitySolver::Vertex> GGGTemporalReachabilitySolver::compute_backwards_temporal_attractor() {
    // Start with target vertices at max_time as initial attractor
    std::set<Vertex> current_attractor;
    
    // Initialize attractor with all target vertices
    auto [vertex_begin, vertex_end] = boost::vertices(*manager_->graph());
    for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
        Vertex vertex = *vertex_it;
        if (objective_->is_target(vertex)) {
            current_attractor.insert(vertex);
        }
    }
    
    if (verbose_) {
        std::cout << "Starting backwards attractor from time " << max_time_ 
                  << " with " << current_attractor.size() << " target vertices: {";
        bool first = true;
        for (auto vertex : current_attractor) {
            if (!first) std::cout << ", ";
            std::cout << (*manager_->graph())[vertex].name;
            first = false;
        }
        std::cout << "}\n";
    }
    
    // Work backwards from max_time to 0
    for (int time = max_time_ - 1; time >= 0; --time) {
        stats_.states_explored++;
        
        std::set<Vertex> new_attractor;
        
        // For each vertex, check if it should be in the attractor at this time
        for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
            Vertex vertex = *vertex_it;
            
            // Get available moves from this vertex at this time
            std::vector<Vertex> moves = manager_->get_available_moves(vertex, time);
            stats_.constraint_evaluations++;
            
            if (moves.empty()) {
                // No moves available - skip this vertex
                stats_.constraint_failures++;
                continue;
            }
            stats_.constraint_passes++;
            
            int player = (*manager_->graph())[vertex].player;
            
            if (player == 0) {
                // Player 0 (existential): needs AT LEAST ONE edge to current attractor
                bool has_edge_to_attractor = false;
                for (auto move : moves) {
                    if (current_attractor.find(move) != current_attractor.end()) {
                        has_edge_to_attractor = true;
                        break;
                    }
                }
                if (has_edge_to_attractor) {
                    new_attractor.insert(vertex);
                }
            } else {
                // Player 1 (universal): needs ALL EDGES to go to current attractor
                bool all_edges_to_attractor = true;
                for (auto move : moves) {
                    if (current_attractor.find(move) == current_attractor.end()) {
                        all_edges_to_attractor = false;
                        break;
                    }
                }
                if (all_edges_to_attractor) {
                    new_attractor.insert(vertex);
                }
            }
        }
        
        // Update current attractor (non-monotonic: replace, don't union)
        current_attractor = new_attractor;
        
        if (verbose_) {
            std::cout << "Time " << time << ": attractor has " << current_attractor.size() << " vertices: {";
            bool first = true;
            for (auto vertex : current_attractor) {
                if (!first) std::cout << ", ";
                std::cout << (*manager_->graph())[vertex].name;
                first = false;
            }
            std::cout << "}\n";
        }
    }
    
    return current_attractor;
}

GGGTemporalReachabilitySolver::SolutionType GGGTemporalReachabilitySolver::build_solution_from_memo(Vertex initial_vertex, int initial_time) {
    SolutionType solution(true);
    TemporalGameState initial_state{initial_vertex, initial_time};
    
    if (memo_.find(initial_state) != memo_.end()) {
        if (memo_[initial_state] > 0) {
            solution.set_winning_player(initial_vertex, 0);
        } else if (memo_[initial_state] < 0) {
            solution.set_winning_player(initial_vertex, 1);
        }
    }
    
    // Build strategies from memo (simplified)
    for (const auto& [state, winner] : memo_) {
        int current_player = (*manager_->graph())[state.vertex].player;
        if ((current_player == 0 && winner > 0) || (current_player == 1 && winner < 0)) {
            auto moves = get_available_moves(state);
            for (auto move : moves) {
                TemporalGameState next_state{move, state.time + 1};
                if (memo_.find(next_state) != memo_.end() && memo_[next_state] == winner) {
                    solution.set_strategy(state.vertex, move);
                    break;
                }
            }
        }
    }
    
    return solution;
}

// TemporalReachabilitySolution implementation
void GGGTemporalReachabilitySolution::add_statistic(const std::string& key, const std::string& value) {
    statistics_[key] = value;
}

std::map<std::string, std::string> GGGTemporalReachabilitySolution::get_statistics() const {
    return statistics_;
}

void GGGTemporalReachabilitySolution::set_time_bound_used(int time_bound) {
    add_statistic("time_bound", std::to_string(time_bound));
}

void GGGTemporalReachabilitySolution::set_states_explored(size_t count) {
    add_statistic("states_explored", std::to_string(count));
}

void GGGTemporalReachabilitySolution::set_memoization_hits(size_t count) {
    add_statistic("memoization_hits", std::to_string(count));
}

} // namespace solvers
} // namespace ggg
