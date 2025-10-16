#include "static_expansion_solver.hpp"
#include "libggg/parity/graph.hpp"
#include <boost/graph/graph_traits.hpp>
#include <iostream>
#include <algorithm>

namespace ggg {
namespace solvers {

StaticExpansionSolver::StaticExpansionSolver(
    std::shared_ptr<graphs::GGGTemporalGameManager> manager,
    std::shared_ptr<graphs::GGGReachabilityObjective> objective,
    int max_time, bool verbose)
    : manager_(manager), objective_(objective), max_time_(max_time), verbose_(verbose) {
}

std::string StaticExpansionSolver::get_name() const {
    return "Static Expansion Temporal Solver";
}

StaticExpansionSolver::SolutionType StaticExpansionSolver::solve(const GraphType& graph) {
    // Reset statistics for this solve
    stats_.reset();
    auto solve_start = std::chrono::high_resolution_clock::now();
    
    // Collect original graph statistics
    stats_.original_vertices = boost::num_vertices(graph);
    stats_.original_edges = boost::num_edges(graph);
    stats_.time_layers = max_time_ + 1; // 0 to max_time inclusive
    
    if (verbose_) {
        std::cout << "=== Static Expansion Solver ===" << std::endl;
        std::cout << "Original graph: " << stats_.original_vertices << " vertices, " 
                  << stats_.original_edges << " edges" << std::endl;
        std::cout << "Time bound: " << max_time_ << " (creating " << stats_.time_layers << " time layers)" << std::endl;
    }
    
    // Step 1: Create expanded graph with static expansion
    auto expansion_start = std::chrono::high_resolution_clock::now();
    ExpandedGraph expanded_graph = create_expanded_graph(graph);
    auto expansion_end = std::chrono::high_resolution_clock::now();
    stats_.expansion_time = expansion_end - expansion_start;
    
    stats_.expanded_vertices = boost::num_vertices(expanded_graph);
    stats_.expanded_edges = boost::num_edges(expanded_graph);
    
    if (verbose_) {
        std::cout << "Expanded graph: " << stats_.expanded_vertices << " vertices, " 
                  << stats_.expanded_edges << " edges" << std::endl;
        std::cout << "Expansion time: " << stats_.expansion_time.count() << "s" << std::endl;
    }
    
    // Step 2: Create target set (target vertices at max_time)
    std::set<ExpandedVertex> target_set = create_target_set(expanded_graph);
    stats_.target_vertices_at_max_time = target_set.size();
    
    if (verbose_) {
        std::cout << "Target vertices at time " << max_time_ << ": " << stats_.target_vertices_at_max_time << std::endl;
    }
    
    // Step 3: Compute attractor for Player 0 on expanded graph
    auto attractor_start = std::chrono::high_resolution_clock::now();
    auto [attractor, strategy] = ggg::graphs::player_utilities::compute_attractor(expanded_graph, target_set, 0);
    auto attractor_end = std::chrono::high_resolution_clock::now();
    stats_.attractor_time = attractor_end - attractor_start;
    
    stats_.attractor_vertices = attractor.size();
    
    if (verbose_) {
        std::cout << "Attractor computation time: " << stats_.attractor_time.count() << "s" << std::endl;
        std::cout << "Attractor size: " << stats_.attractor_vertices << " vertices" << std::endl;
    }
    
    // Step 4: Convert result back to temporal solution
    SolutionType solution = convert_attractor_to_solution(graph, attractor, strategy);
    
    // Record total solve time
    auto solve_end = std::chrono::high_resolution_clock::now();
    stats_.total_solve_time = solve_end - solve_start;
    
    if (verbose_) {
        std::cout << "Vertices winning at time 0: " << stats_.vertices_winning_at_time_0 << std::endl;
        std::cout << "Total solve time: " << stats_.total_solve_time.count() << "s" << std::endl;
        std::cout << "Constraint evaluations: " << stats_.constraint_evaluations 
                  << " (passed: " << stats_.constraint_passes 
                  << ", failed: " << stats_.constraint_failures << ")" << std::endl;
    }
    
    return solution;
}

StaticExpansionSolver::ExpandedGraph StaticExpansionSolver::create_expanded_graph(const GraphType& temporal_graph) {
    ExpandedGraph expanded_graph;
    
    // Clear mappings
    temporal_to_expanded_.clear();
    expanded_to_temporal_.clear();
    
    // Step 1: Create vertices for all time layers
    create_time_layers(temporal_graph, expanded_graph);
    
    // Step 2: Add edges between time layers based on temporal constraints
    add_temporal_edges(temporal_graph, expanded_graph);
    
    return expanded_graph;
}

void StaticExpansionSolver::create_time_layers(const GraphType& temporal_graph, ExpandedGraph& expanded_graph) {
    if (verbose_) {
        std::cout << "Creating time layers..." << std::endl;
    }
    
    // For each time step from 0 to max_time
    for (int time = 0; time <= max_time_; ++time) {
        // For each vertex in the original temporal graph
        auto [vertex_begin, vertex_end] = boost::vertices(temporal_graph);
        for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
            TemporalVertex temporal_vertex = *vertex_it;
            
            // Create expanded vertex name: original_name_t<time>
            std::string expanded_name = temporal_graph[temporal_vertex].name + "_t" + std::to_string(time);
            
            // Copy player from original vertex, use time as priority
            int player = temporal_graph[temporal_vertex].player;
            int priority = time; // Priority represents time layer
            
            // Create vertex in expanded graph
            ExpandedVertex expanded_vertex = ggg::parity::graph::add_vertex(expanded_graph, expanded_name, player, priority);
            
            // Store mappings
            temporal_to_expanded_[{temporal_vertex, time}] = expanded_vertex;
            expanded_to_temporal_[expanded_vertex] = {temporal_vertex, time};
        }
    }
    
    if (verbose_) {
        std::cout << "Created " << boost::num_vertices(expanded_graph) << " vertices across " 
                  << (max_time_ + 1) << " time layers" << std::endl;
    }
}

void StaticExpansionSolver::add_temporal_edges(const GraphType& temporal_graph, ExpandedGraph& expanded_graph) {
    if (verbose_) {
        std::cout << "Adding temporal edges..." << std::endl;
    }
    
    // For each time step (edges go from time t to time t+1)
    for (int time = 0; time < max_time_; ++time) {
        // For each edge in the original temporal graph
        auto [edge_begin, edge_end] = boost::edges(temporal_graph);
        for (auto edge_it = edge_begin; edge_it != edge_end; ++edge_it) {
            auto temporal_edge = *edge_it;
            TemporalVertex source = boost::source(temporal_edge, temporal_graph);
            TemporalVertex target = boost::target(temporal_edge, temporal_graph);
            
            stats_.constraint_evaluations++;
            
            // Check if this edge is available at this time using temporal constraints
            bool edge_available = manager_->is_edge_constraint_satisfied(temporal_edge, time);
            
            if (edge_available) {
                stats_.constraint_passes++;
                
                // Get corresponding vertices in expanded graph
                ExpandedVertex source_expanded = temporal_to_expanded_[{source, time}];
                ExpandedVertex target_expanded = temporal_to_expanded_[{target, time + 1}];
                
                // Add edge in expanded graph
                std::string edge_label = temporal_graph[temporal_edge].label + "_t" + std::to_string(time);
                ggg::parity::graph::add_edge(expanded_graph, source_expanded, target_expanded, edge_label);
            } else {
                stats_.constraint_failures++;
            }
        }
    }
    
    if (verbose_) {
        std::cout << "Added " << boost::num_edges(expanded_graph) << " temporal edges" << std::endl;
        std::cout << "Constraint evaluations: " << stats_.constraint_evaluations 
                  << " (passed: " << stats_.constraint_passes 
                  << ", failed: " << stats_.constraint_failures << ")" << std::endl;
    }
}

std::set<StaticExpansionSolver::ExpandedVertex> StaticExpansionSolver::create_target_set(const ExpandedGraph& expanded_graph) {
    std::set<ExpandedVertex> target_set;
    
    // Target vertices are the temporal target vertices at max_time
    auto temporal_targets = objective_->get_targets();
    
    for (TemporalVertex temporal_target : temporal_targets) {
        auto key = std::make_pair(temporal_target, max_time_);
        auto it = temporal_to_expanded_.find(key);
        if (it != temporal_to_expanded_.end()) {
            target_set.insert(it->second);
        }
    }
    
    if (verbose_) {
        std::cout << "Target set contains " << target_set.size() << " vertices at time " << max_time_ << ": {";
        bool first = true;
        for (ExpandedVertex target : target_set) {
            if (!first) std::cout << ", ";
            std::cout << expanded_graph[target].name;
            first = false;
        }
        std::cout << "}" << std::endl;
    }
    
    return target_set;
}

StaticExpansionSolver::SolutionType StaticExpansionSolver::convert_attractor_to_solution(
    const GraphType& temporal_graph,
    const std::set<ExpandedVertex>& attractor,
    const std::map<ExpandedVertex, ExpandedVertex>& strategy) {
    
    SolutionType solution;
    
    // For each vertex in the original temporal graph
    auto [vertex_begin, vertex_end] = boost::vertices(temporal_graph);
    for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
        TemporalVertex temporal_vertex = *vertex_it;
        
        // Check if this vertex at time 0 is in the attractor
        auto key = std::make_pair(temporal_vertex, 0);
        auto it = temporal_to_expanded_.find(key);
        
        if (it != temporal_to_expanded_.end()) {
            ExpandedVertex expanded_vertex_at_time_0 = it->second;
            
            if (attractor.find(expanded_vertex_at_time_0) != attractor.end()) {
                // This vertex is winning for Player 0
                solution.set_winning_player(temporal_vertex, 0);
                stats_.vertices_winning_at_time_0++;
                
                // Set strategy if available
                auto strategy_it = strategy.find(expanded_vertex_at_time_0);
                if (strategy_it != strategy.end()) {
                    ExpandedVertex strategy_target = strategy_it->second;
                    
                    // Find corresponding temporal vertex for strategy
                    auto target_it = expanded_to_temporal_.find(strategy_target);
                    if (target_it != expanded_to_temporal_.end()) {
                        TemporalVertex temporal_strategy_target = target_it->second.first;
                        solution.set_strategy(temporal_vertex, temporal_strategy_target);
                    }
                }
            } else {
                // This vertex is winning for Player 1
                solution.set_winning_player(temporal_vertex, 1);
            }
        } else {
            // Safety fallback: vertex not found, assign to Player 1
            solution.set_winning_player(temporal_vertex, 1);
        }
    }
    
    if (verbose_) {
        std::cout << "Solution extracted: " << stats_.vertices_winning_at_time_0 
                  << " vertices winning for Player 0 at time 0" << std::endl;
    }
    
    return solution;
}

} // namespace solvers
} // namespace ggg
