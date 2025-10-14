#pragma once

#include "ggg_temporal_graph.hpp"
#include "libggg/solvers/solver.hpp"
#include <map>
#include <set>
#include <memory>
#include <chrono>

namespace ggg {
namespace solvers {

/**
 * @brief Performance and debugging statistics for temporal solver
 */
struct SolverStatistics {
    // State space exploration
    size_t states_explored = 0;
    size_t states_pruned = 0;
    size_t max_time_reached = 0;
    
    // Constraint evaluation 
    size_t constraint_evaluations = 0;
    size_t constraint_passes = 0;
    size_t constraint_failures = 0;
    
    // Memoization performance
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    
    // Timing
    std::chrono::duration<double> total_solve_time{0};
    std::chrono::duration<double> constraint_eval_time{0};
    std::chrono::duration<double> graph_traversal_time{0};
    
    // Reset all statistics
    void reset() {
        states_explored = states_pruned = max_time_reached = 0;
        constraint_evaluations = constraint_passes = constraint_failures = 0;
        cache_hits = cache_misses = 0;
        total_solve_time = constraint_eval_time = graph_traversal_time = std::chrono::duration<double>{0};
    }
    
    // Get cache hit ratio (0.0 to 1.0)
    double cache_hit_ratio() const {
        size_t total = cache_hits + cache_misses;
        return total > 0 ? static_cast<double>(cache_hits) / total : 0.0;
    }
    
    // Get constraint success ratio (0.0 to 1.0)
    double constraint_success_ratio() const {
        return constraint_evaluations > 0 ? static_cast<double>(constraint_passes) / constraint_evaluations : 0.0;
    }
};

/**
 * @brief GGG-compatible solver for temporal reachability games
 * 
 * Implements the standard GGG Solver interface while providing specialized
 * temporal game solving capabilities with Presburger arithmetic constraints.
 */
class GGGTemporalReachabilitySolver : public Solver<graphs::GGGTemporalGraph, solutions::RSSolution<graphs::GGGTemporalGraph>> {
public:
    using GraphType = graphs::GGGTemporalGraph;
    using SolutionType = solutions::RSSolution<GraphType>;
    using Vertex = typename boost::graph_traits<GraphType>::vertex_descriptor;

private:
    std::shared_ptr<graphs::GGGTemporalGameManager> manager_;
    std::shared_ptr<graphs::GGGReachabilityObjective> objective_;
    int max_time_;
    bool verbose_;
    
    // Performance and debugging statistics
    mutable SolverStatistics stats_;

public:
    /**
     * @brief Construct solver with game manager and objective
     */
    GGGTemporalReachabilitySolver(std::shared_ptr<graphs::GGGTemporalGameManager> manager,
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
     * @brief Solve from specific initial state
     */
    SolutionType solve_from_state(Vertex initial_vertex, int initial_time = 0);
    
    /**
     * @brief Get solver performance statistics
     */
    const SolverStatistics& get_statistics() const { return stats_; }
    
    /**
     * @brief Reset solver statistics
     */
    void reset_statistics() { stats_.reset(); }

private:
    /**
     * @brief Compute backwards temporal attractor starting from targets at max_time
     */
    std::set<Vertex> compute_backwards_temporal_attractor();
};

/**
 * @brief Extended solution type with temporal game statistics
 */
class GGGTemporalReachabilitySolution : public solutions::RSSolution<graphs::GGGTemporalGraph> {
private:
    std::map<std::string, std::string> statistics_;
    
public:
    GGGTemporalReachabilitySolution() = default;
    explicit GGGTemporalReachabilitySolution(bool solved, bool valid = true) 
        : RSSolution<graphs::GGGTemporalGraph>(solved, valid) {}
    
    /**
     * @brief Add a statistic to the solution
     */
    void add_statistic(const std::string& key, const std::string& value);
    
    /**
     * @brief Get all statistics (GGG-compatible interface)
     */
    std::map<std::string, std::string> get_statistics() const;
    
    /**
     * @brief Set temporal-specific data
     */
    void set_time_bound_used(int time_bound);
    void set_states_explored(size_t count);
    void set_memoization_hits(size_t count);
};

} // namespace solvers
} // namespace ggg
