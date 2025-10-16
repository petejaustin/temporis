#pragma once

#include "ggg_temporal_graph.hpp"
#include "libggg/solvers/solver.hpp"
#include "libggg/graphs/player_utilities.hpp"
#include "libggg/parity/graph.hpp"
#include <map>
#include <set>
#include <memory>
#include <chrono>

namespace ggg {
namespace solvers {

/**
 * @brief Performance statistics for static expansion solver
 */
struct StaticExpansionStatistics {
    // Static expansion metrics
    size_t original_vertices = 0;
    size_t original_edges = 0;
    size_t expanded_vertices = 0;
    size_t expanded_edges = 0;
    size_t time_layers = 0;
    
    // Constraint evaluation
    size_t constraint_evaluations = 0;
    size_t constraint_passes = 0;
    size_t constraint_failures = 0;
    
    // Attractor computation
    size_t target_vertices_at_max_time = 0;
    size_t attractor_vertices = 0;
    size_t vertices_winning_at_time_0 = 0;
    
    // Timing
    std::chrono::duration<double> total_solve_time{0};
    std::chrono::duration<double> expansion_time{0};
    std::chrono::duration<double> attractor_time{0};
    
    void reset() {
        original_vertices = original_edges = 0;
        expanded_vertices = expanded_edges = 0;
        time_layers = 0;
        constraint_evaluations = constraint_passes = constraint_failures = 0;
        target_vertices_at_max_time = attractor_vertices = vertices_winning_at_time_0 = 0;
        total_solve_time = expansion_time = attractor_time = std::chrono::duration<double>{0};
    }
};

/**
 * @brief Static expansion temporal solver
 * 
 * This solver implements the following approach:
 * 1. Performs a STATIC EXPANSION of the temporal graph into a conventional graph
 * 2. Creates vertices for each (original_vertex, time) pair from 0 to max_time
 * 3. Adds edges between time layers based on temporal constraints
 * 4. Uses GGG's attractor computation on the expanded graph
 * 5. Extracts solution by checking which vertices at time 0 are in the attractor
 */
class StaticExpansionSolver : public Solver<graphs::GGGTemporalGraph, solutions::RSSolution<graphs::GGGTemporalGraph>> {
public:
    using GraphType = graphs::GGGTemporalGraph;
    using SolutionType = solutions::RSSolution<GraphType>;
    using TemporalVertex = typename boost::graph_traits<GraphType>::vertex_descriptor;
    
    // Expanded graph will use parity graph structure (player, priority=time)
    using ExpandedGraph = ggg::parity::graph::Graph;
    using ExpandedVertex = typename boost::graph_traits<ExpandedGraph>::vertex_descriptor;

private:
    std::shared_ptr<graphs::GGGTemporalGameManager> manager_;
    std::shared_ptr<graphs::GGGReachabilityObjective> objective_;
    int max_time_;
    bool verbose_;
    
    // Performance statistics
    mutable StaticExpansionStatistics stats_;
    
    // Mapping between temporal and expanded vertices
    std::map<std::pair<TemporalVertex, int>, ExpandedVertex> temporal_to_expanded_;
    std::map<ExpandedVertex, std::pair<TemporalVertex, int>> expanded_to_temporal_;

public:
    /**
     * @brief Construct static expansion solver
     */
    StaticExpansionSolver(std::shared_ptr<graphs::GGGTemporalGameManager> manager,
                         std::shared_ptr<graphs::GGGReachabilityObjective> objective,
                         int max_time = 50, bool verbose = false);

    /**
     * @brief GGG Solver interface implementation
     */
    SolutionType solve(const GraphType& graph) override;
    
    /**
     * @brief GGG Solver interface implementation
     */
    std::string get_name() const override;
    
    /**
     * @brief Get solver performance statistics
     */
    const StaticExpansionStatistics& get_statistics() const { return stats_; }
    
    /**
     * @brief Reset solver statistics
     */
    void reset_statistics() { stats_.reset(); }

private:
    /**
     * @brief Perform static expansion of temporal graph
     * @return Expanded graph with all time layers
     */
    ExpandedGraph create_expanded_graph(const GraphType& temporal_graph);
    
    /**
     * @brief Create vertices for all time layers
     */
    void create_time_layers(const GraphType& temporal_graph, ExpandedGraph& expanded_graph);
    
    /**
     * @brief Add edges between time layers based on temporal constraints
     */
    void add_temporal_edges(const GraphType& temporal_graph, ExpandedGraph& expanded_graph);
    
    /**
     * @brief Create target vertices at max_time
     */
    std::set<ExpandedVertex> create_target_set(const ExpandedGraph& expanded_graph);
    
    /**
     * @brief Convert attractor result back to temporal solution
     */
    SolutionType convert_attractor_to_solution(const GraphType& temporal_graph,
                                              const std::set<ExpandedVertex>& attractor,
                                              const std::map<ExpandedVertex, ExpandedVertex>& strategy);
};

} // namespace solvers
} // namespace ggg
