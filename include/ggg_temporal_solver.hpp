#pragma once

#include "ggg_temporal_graph.hpp"
#include "libggg/solvers/solver.hpp"
#include <map>
#include <set>
#include <memory>

namespace ggg {
namespace solvers {

/**
 * @brief Game state combining vertex and time for temporal analysis
 */
struct TemporalGameState {
    graphs::GGGTemporalVertex vertex;
    int time;
    
    bool operator<(const TemporalGameState& other) const {
        if (vertex != other.vertex) return vertex < other.vertex;
        return time < other.time;
    }
    
    bool operator==(const TemporalGameState& other) const {
        return vertex == other.vertex && time == other.time;
    }
};

/**
 * @brief GGG-compatible solver for temporal reachability games
 * 
 * Implements the standard GGG Solver interface while providing specialized
 * temporal game solving capabilities with Presburger arithmetic constraints.
 */
class GGGTemporalReachabilitySolver : public Solver<graphs::GGGTemporalGraph, RSSolution<graphs::GGGTemporalGraph>> {
public:
    using GraphType = graphs::GGGTemporalGraph;
    using SolutionType = RSSolution<GraphType>;
    using Vertex = typename boost::graph_traits<GraphType>::vertex_descriptor;

private:
    std::shared_ptr<graphs::GGGTemporalGameManager> manager_;
    std::shared_ptr<graphs::GGGReachabilityObjective> objective_;
    int max_time_;
    bool verbose_;
    
    // Memoization for dynamic programming
    std::map<TemporalGameState, int> memo_; // -1 = Player 1 wins, 1 = Player 0 wins, 0 = draw

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
     * @brief Compute winning regions for all vertices at given time
     */
    std::pair<std::set<Vertex>, std::set<Vertex>> compute_winning_regions(int initial_time = 0);

private:
    /**
     * @brief Recursive solver using minimax with memoization
     */
    int solve_recursive(const TemporalGameState& state, std::set<TemporalGameState>& visited);
    
    /**
     * @brief Check if a state is terminal
     */
    bool is_terminal_state(const TemporalGameState& state, int& winner);
    
    /**
     * @brief Get available moves from a state
     */
    std::vector<Vertex> get_available_moves(const TemporalGameState& state);
    
    /**
     * @brief Build solution from memoized results
     */
    SolutionType build_solution_from_memo(Vertex initial_vertex, int initial_time);
};

/**
 * @brief Extended solution type with temporal game statistics
 */
class GGGTemporalReachabilitySolution : public RSSolution<graphs::GGGTemporalGraph> {
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
