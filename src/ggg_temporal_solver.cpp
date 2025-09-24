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
            
            // Build basic strategy: pick first available move at time 0
            auto moves = manager_->get_available_moves(vertex, 0);
            if (!moves.empty()) {
                solution.set_strategy(vertex, moves[0]);
            }
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
    
    // Start with target vertices as initial attractor
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

// ==== GGGTemporalExpansionSolver Implementation ====

GGGTemporalExpansionSolver::GGGTemporalExpansionSolver(
    std::shared_ptr<graphs::GGGTemporalGameManager> manager,
    std::shared_ptr<graphs::GGGReachabilityObjective> objective,
    int max_time, bool verbose)
    : manager_(manager), objective_(objective), max_time_(max_time), verbose_(verbose) {
}

std::string GGGTemporalExpansionSolver::get_name() const {
    return "Temporal Expansion Solver";
}

GGGTemporalExpansionSolver::SolutionType GGGTemporalExpansionSolver::solve(const GraphType& graph) {
    stats_.reset();
    auto solve_start = std::chrono::high_resolution_clock::now();
    
    if (verbose_) {
        std::cout << "Starting temporal expansion solver with max_time=" << max_time_ << std::endl;
    }
    
    // Step 1: Expand temporal graph into static graph
    auto static_graph = expand_temporal_graph(graph);
    
    // Step 2: Create target set (targets only at max time)
    auto target_set = create_expanded_target_set(*static_graph);
    
    if (verbose_) {
        std::cout << "Expanded to " << boost::num_vertices(*static_graph) << " vertices, " 
                  << boost::num_edges(*static_graph) << " edges" << std::endl;
        std::cout << "Target set size: " << target_set.size() << std::endl;
    }
    
    // Step 3: Compute attractor using simple backwards reachability
    // For now, implement a simple backwards reachability algorithm
    // In a full implementation, you'd use GGG's attractor functions
    std::set<StaticVertex> winning_vertices = compute_static_attractor(*static_graph, target_set);
    
    // Step 4: Convert solution back to original graph
    auto solution = convert_solution_back(winning_vertices, graph);
    
    auto solve_end = std::chrono::high_resolution_clock::now();
    stats_.total_solve_time = solve_end - solve_start;
    
    if (verbose_) {
        std::cout << "Expansion solve completed in " << stats_.total_solve_time.count() << " seconds" << std::endl;
    }
    
    return solution;
}

std::shared_ptr<GGGTemporalExpansionSolver::StaticGraph> 
GGGTemporalExpansionSolver::expand_temporal_graph(const GraphType& temporal_graph) {
    auto static_graph = std::make_shared<StaticGraph>();
    vertex_map_.clear();
    reverse_map_.clear();
    
    // Step 1: Create time-indexed vertices
    auto [vertex_begin, vertex_end] = boost::vertices(temporal_graph);
    for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
        Vertex original_vertex = *vertex_it;
        
        for (int time = 0; time <= max_time_; ++time) {
            // Create new vertex in static graph
            StaticVertex static_vertex = boost::add_vertex(*static_graph);
            
            // Create vertex name for debugging
            std::string vertex_name = temporal_graph[original_vertex].name + "@" + std::to_string(time);
            boost::put(boost::vertex_name, *static_graph, static_vertex, vertex_name);
            
            // Store mappings
            vertex_map_[{original_vertex, time}] = static_vertex;
            reverse_map_[static_vertex] = {original_vertex, time};
        }
    }
    
    // Step 2: Create edges based on temporal constraints
    auto [edge_begin, edge_end] = boost::edges(temporal_graph);
    for (auto edge_it = edge_begin; edge_it != edge_end; ++edge_it) {
        auto edge = *edge_it;
        Vertex source = boost::source(edge, temporal_graph);
        Vertex target = boost::target(edge, temporal_graph);
        
        // For each time step, check if edge constraint is satisfied
        for (int time = 0; time < max_time_; ++time) { // Note: time < max_time (can't move from max_time)
            if (manager_->is_edge_constraint_satisfied(edge, time)) {
                // Add edge from source@time to target@(time+1)
                StaticVertex static_source = vertex_map_[{source, time}];
                StaticVertex static_target = vertex_map_[{target, time + 1}];
                
                boost::add_edge(static_source, static_target, *static_graph);
                stats_.states_explored++;
            }
        }
    }
    
    return static_graph;
}

std::set<GGGTemporalExpansionSolver::StaticVertex> 
GGGTemporalExpansionSolver::create_expanded_target_set(const StaticGraph& static_graph) {
    std::set<StaticVertex> target_set;
    
    // Get original target vertices
    auto original_targets = objective_->get_targets();
    
    // Add only the max-time versions of target vertices
    for (Vertex original_target : original_targets) {
        auto key = std::make_pair(original_target, max_time_);
        auto it = vertex_map_.find(key);
        if (it != vertex_map_.end()) {
            target_set.insert(it->second);
        }
    }
    
    return target_set;
}

std::set<GGGTemporalExpansionSolver::StaticVertex>
GGGTemporalExpansionSolver::compute_static_attractor(const StaticGraph& static_graph, 
                                                     const std::set<StaticVertex>& target_set) {
    // Simple backwards reachability algorithm
    std::set<StaticVertex> attractor = target_set;
    std::set<StaticVertex> new_vertices;
    
    bool changed = true;
    while (changed) {
        changed = false;
        new_vertices.clear();
        
        auto [vertex_begin, vertex_end] = boost::vertices(static_graph);
        for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
            StaticVertex vertex = *vertex_it;
            
            if (attractor.find(vertex) != attractor.end()) {
                continue; // Already in attractor
            }
            
            // Check if vertex can reach attractor
            auto [out_begin, out_end] = boost::out_edges(vertex, static_graph);
            for (auto edge_it = out_begin; edge_it != out_end; ++edge_it) {
                StaticVertex successor = boost::target(*edge_it, static_graph);
                if (attractor.find(successor) != attractor.end()) {
                    new_vertices.insert(vertex);
                    changed = true;
                    break;
                }
            }
        }
        
        attractor.insert(new_vertices.begin(), new_vertices.end());
    }
    
    return attractor;
}

GGGTemporalExpansionSolver::SolutionType 
GGGTemporalExpansionSolver::convert_solution_back(const std::set<StaticVertex>& winning_vertices, 
                                                  const GraphType& original_graph) {
    SolutionType solution(true);
    
    // IMPORTANT: Only consider vertices at time 0 for the solution
    // The attractor computation gives us winning vertices at all time indices,
    // but we only care about which original vertices are winning when the game starts (at time 0)
    std::set<Vertex> player0_winning;
    
    auto [vertex_begin, vertex_end] = boost::vertices(original_graph);
    for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
        Vertex original_vertex = *vertex_it;
        
        // Check if this vertex at time 0 is in the winning set from attractor computation
        auto key = std::make_pair(original_vertex, 0);
        auto it = vertex_map_.find(key);
        if (it != vertex_map_.end() && winning_vertices.find(it->second) != winning_vertices.end()) {
            player0_winning.insert(original_vertex);
        }
    }
    
    // Set winning regions in solution based only on time-0 analysis
    for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
        Vertex vertex = *vertex_it;
        
        if (player0_winning.find(vertex) != player0_winning.end()) {
            solution.set_winning_player(vertex, 0);
            
            // Build strategy: pick first available move at time 0
            // (In a more sophisticated implementation, we could trace back through 
            // the expanded graph to find optimal moves at each time step)
            auto moves = manager_->get_available_moves(vertex, 0);
            if (!moves.empty()) {
                solution.set_strategy(vertex, moves[0]);
            }
        } else {
            solution.set_winning_player(vertex, 1);
        }
    }
    
    return solution;
}

} // namespace solvers
} // namespace ggg
