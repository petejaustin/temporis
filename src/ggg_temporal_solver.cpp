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
    
    // Compute backwards temporal attractor
    std::set<Vertex> player0_winning = compute_backwards_temporal_attractor();
    
    // Build solution
    SolutionType solution(true);
    
    // Set winning regions
    auto [vertex_begin, vertex_end] = boost::vertices(graph);
    for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
        Vertex vertex = *vertex_it;
        
        if (player0_winning.find(vertex) != player0_winning.end()) {
            solution.set_winning_player(vertex, 0);
            
            // Build strategy: find the time when Player 0 should make a move
            // For punctual reachability, look for the earliest time with available moves
            // that lead toward the target
            bool strategy_found = false;
            for (int t = 0; t < max_time_ && !strategy_found; ++t) {
                auto moves = manager_->get_available_moves(vertex, t);
                if (!moves.empty()) {
                    // Pick the first available move (could be improved with better heuristics)
                    solution.set_strategy(vertex, moves[0]);
                    strategy_found = true;
                }
            }
            // If no strategy found, don't set any strategy (indicates staying/no moves)
        } else {
            solution.set_winning_player(vertex, 1);
        }
    }
    
    // Record total solve time
    auto solve_end = std::chrono::high_resolution_clock::now();
    stats_.total_solve_time = solve_end - solve_start;
    
    return solution;
}

GGGTemporalReachabilitySolver::SolutionType GGGTemporalReachabilitySolver::solve_from_state(Vertex initial_vertex, int initial_time) {
    // For backwards attractor, just solve the entire game and extract result for this vertex
    return solve(*manager_->graph());
}

std::set<GGGTemporalReachabilitySolver::Vertex> GGGTemporalReachabilitySolver::compute_backwards_temporal_attractor() {
    // Time the graph traversal
    auto traversal_start = std::chrono::high_resolution_clock::now();
    
    // Start with empty attractor for punctual reachability
    // In punctual reachability, vertices must be actively reachable through gameplay
    std::set<Vertex> current_attractor;
    
    auto [vertex_begin, vertex_end] = boost::vertices(*manager_->graph());
    
    if (verbose_) {
        std::cout << "Starting backwards attractor from time " << max_time_ 
                  << " with empty initial attractor (punctual reachability)\n";
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
                // No moves available - in punctual reachability, this means the player
                // cannot actively reach the target set through gameplay, so this vertex
                // should NOT be in the attractor (even if it's a target vertex)
                stats_.constraint_failures++;
                continue;
            }
            stats_.constraint_passes++;
            
            int player = (*manager_->graph())[vertex].player;
            
            // Special case: if we're at max_time-1, check if moves lead to targets
            if (time == max_time_ - 1) {
                if (player == 0) {
                    // Player 0: needs at least one move to a target
                    bool has_move_to_target = false;
                    for (auto move : moves) {
                        if (objective_->is_target(move)) {
                            has_move_to_target = true;
                            break;
                        }
                    }
                    if (has_move_to_target) {
                        new_attractor.insert(vertex);
                    }
                } else {
                    // Player 1: all moves must lead to targets (for this to help Player 0)
                    bool all_moves_to_targets = true;
                    for (auto move : moves) {
                        if (!objective_->is_target(move)) {
                            all_moves_to_targets = false;
                            break;
                        }
                    }
                    if (all_moves_to_targets) {
                        new_attractor.insert(vertex);
                    }
                }
            } else {
                // Standard attractor computation for earlier times
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
    
    // Record timing and final verbose output
    auto traversal_end = std::chrono::high_resolution_clock::now();
    stats_.graph_traversal_time += (traversal_end - traversal_start);
    
    if (verbose_) {
        std::cout << "Final attractor at time 0 has " << current_attractor.size() << " vertices: {";
        bool first = true;
        for (auto vertex : current_attractor) {
            if (!first) std::cout << ", ";
            std::cout << (*manager_->graph())[vertex].name;
            first = false;
        }
        std::cout << "}\n";
    }
    
    return current_attractor;
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
