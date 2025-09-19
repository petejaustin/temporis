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
    return "Temporal Reachability Solver (Presburger Arithmetic)";
}

GGGTemporalReachabilitySolver::SolutionType GGGTemporalReachabilitySolver::solve(const GraphType& graph) {
    // Compute winning regions from initial time
    auto [player0_winning, player1_winning] = compute_winning_regions(0);
    
    // "Solved" means we successfully computed winning regions for all vertices
    // This is independent of who wins the game
    SolutionType solution(true);
    
    // Set winning regions
    for (auto vertex : player0_winning) {
        solution.set_winning_player(vertex, 0);
    }
    for (auto vertex : player1_winning) {
        solution.set_winning_player(vertex, 1);
    }
    
    // Build strategies from memoization
    // This is a simplified strategy extraction - in practice, you'd want more sophisticated strategy synthesis
    auto [vertex_begin, vertex_end] = boost::vertices(graph);
    for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
        Vertex vertex = *vertex_it;
        TemporalGameState state{vertex, 0};
        
        if (memo_.find(state) != memo_.end() && memo_[state] > 0) {
            // Player 0 wins from this state, find a winning move
            auto moves = get_available_moves(state);
            for (auto move : moves) {
                TemporalGameState next_state{move, 1};
                if (memo_.find(next_state) != memo_.end() && memo_[next_state] > 0) {
                    solution.set_strategy(vertex, move);
                    break;
                }
            }
        }
    }
    
    return solution;
}

GGGTemporalReachabilitySolver::SolutionType GGGTemporalReachabilitySolver::solve_from_state(Vertex initial_vertex, int initial_time) {
    memo_.clear();
    TemporalGameState initial_state{initial_vertex, initial_time};
    std::set<TemporalGameState> visited;
    
    int winner = solve_recursive(initial_state, visited);
    
    return build_solution_from_memo(initial_vertex, initial_time);
}

std::pair<std::set<GGGTemporalReachabilitySolver::Vertex>, std::set<GGGTemporalReachabilitySolver::Vertex>>
GGGTemporalReachabilitySolver::compute_winning_regions(int initial_time) {
    std::set<Vertex> player0_winning;
    std::set<Vertex> player1_winning;
    
    memo_.clear();
    
    auto [vertex_begin, vertex_end] = boost::vertices(*manager_->graph());
    
    for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
        Vertex vertex = *vertex_it;
        TemporalGameState state{vertex, initial_time};
        std::set<TemporalGameState> visited;
        
        // For each vertex as starting position, determine who wins
        // If starting vertex is a target, Player 0 wins trivially
        if (objective_->is_target(vertex)) {
            player0_winning.insert(vertex);
            continue;
        }
        
        // Otherwise, run game tree search to see if Player 0 can reach a target
        int winner = solve_recursive(state, visited);
        
        if (winner > 0) {
            // Player 0 can reach a target starting from this vertex
            player0_winning.insert(vertex);
        } else {
            // Player 1 can prevent Player 0 from reaching a target
            player1_winning.insert(vertex);
        }
        // winner == 0 means draw/undetermined
    }
    
    return {player0_winning, player1_winning};
}

int GGGTemporalReachabilitySolver::solve_recursive(const TemporalGameState& state, std::set<TemporalGameState>& visited) {
    // Check memoization
    if (memo_.find(state) != memo_.end()) {
        return memo_[state];
    }
    
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
    return manager_->get_available_moves(state.vertex, state.time);
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
