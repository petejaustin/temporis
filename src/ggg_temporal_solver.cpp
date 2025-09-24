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

// Expansion solver implementation

ExpansionSolver::ExpansionSolver(
    std::shared_ptr<graphs::GGGTemporalGameManager> manager,
    std::shared_ptr<graphs::GGGReachabilityObjective> objective,
    int max_time, bool verbose)
    : manager_(manager), objective_(objective), max_time_(max_time), verbose_(verbose) {}

std::string ExpansionSolver::get_name() const {
    return "Expansion Solver";
}

ExpansionSolver::Solution ExpansionSolver::solve(const Graph& graph) {
    stats_.reset();
    auto start = std::chrono::high_resolution_clock::now();
    
    if (verbose_) std::cout << "Expanding to time " << max_time_ << std::endl;
    
    auto static_graph = expand_graph(graph);
    auto targets = create_targets(*static_graph);
    auto winning = compute_attractor(*static_graph, targets);
    auto solution = convert_back(winning, graph);
    
    stats_.total_solve_time = std::chrono::high_resolution_clock::now() - start;
    if (verbose_) {
        std::cout << "Expanded to " << boost::num_vertices(*static_graph) << " vertices, " 
                  << boost::num_edges(*static_graph) << " edges" << std::endl;
    }
    
    return solution;
}

std::shared_ptr<ExpansionSolver::StaticGraph> ExpansionSolver::expand_graph(const Graph& g) {
    auto sg = std::make_shared<StaticGraph>();
    vertex_map_.clear();
    reverse_map_.clear();
    
    // Create vertices: v@0, v@1, ..., v@max_time
    auto [vb, ve] = boost::vertices(g);
    for (auto v = vb; v != ve; ++v) {
        for (int t = 0; t <= max_time_; ++t) {
            StaticVertex sv = boost::add_vertex(*sg);
            vertex_map_[{*v, t}] = sv;
            reverse_map_[sv] = {*v, t};
        }
    }
    
    // Create edges where constraints hold
    auto [eb, ee] = boost::edges(g);
    for (auto e = eb; e != ee; ++e) {
        Vertex src = boost::source(*e, g);
        Vertex tgt = boost::target(*e, g);
        
        for (int t = 0; t < max_time_; ++t) {
            if (manager_->is_edge_constraint_satisfied(*e, t)) {
                boost::add_edge(vertex_map_[{src, t}], vertex_map_[{tgt, t + 1}], *sg);
                stats_.states_explored++;
            }
        }
    }
    
    return sg;
}

std::set<ExpansionSolver::StaticVertex> ExpansionSolver::create_targets(const StaticGraph& sg) {
    std::set<StaticVertex> targets;
    for (Vertex v : objective_->get_targets()) {
        auto it = vertex_map_.find({v, max_time_});
        if (it != vertex_map_.end()) {
            targets.insert(it->second);
        }
    }
    return targets;
}

std::set<ExpansionSolver::StaticVertex> ExpansionSolver::compute_attractor(
    const StaticGraph& sg, const std::set<StaticVertex>& targets) {
    
    std::set<StaticVertex> attractor = targets;
    bool changed = true;
    
    while (changed) {
        changed = false;
        auto [vb, ve] = boost::vertices(sg);
        
        for (auto v = vb; v != ve; ++v) {
            if (attractor.count(*v)) continue;
            
            auto [eb, ee] = boost::out_edges(*v, sg);
            for (auto e = eb; e != ee; ++e) {
                if (attractor.count(boost::target(*e, sg))) {
                    attractor.insert(*v);
                    changed = true;
                    break;
                }
            }
        }
    }
    
    return attractor;
}

ExpansionSolver::Solution ExpansionSolver::convert_back(
    const std::set<StaticVertex>& winning, const Graph& g) {
    
    Solution sol(true);
    
    // Only care about winning at time 0
    auto [vb, ve] = boost::vertices(g);
    for (auto v = vb; v != ve; ++v) {
        auto it = vertex_map_.find({*v, 0});
        bool wins = (it != vertex_map_.end() && winning.count(it->second));
        
        sol.set_winning_player(*v, wins ? 0 : 1);
        
        if (wins) {
            auto moves = manager_->get_available_moves(*v, 0);
            if (!moves.empty()) {
                sol.set_strategy(*v, moves[0]);
            }
        }
    }
    
    return sol;
}

} // namespace solvers
} // namespace ggg
