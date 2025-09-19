#pragma once

#include "ggg_temporal_graph.hpp"
#include <set>
#include <string>

using namespace ggg::graphs;

/**
 * @brief Represents a reachability objective in temporal games
 * 
 * Defines target vertices that Player 0 tries to reach and Player 1 tries to avoid.
 * Supports various reachability conditions including time-bounded reachability.
 */
class ReachabilityObjective {
public:
    /**
     * @brief Type of reachability objective
     */
    enum class ObjectiveType {
        REACHABILITY,           // Reach target eventually
        SAFETY,                 // Avoid target forever  
        TIME_BOUNDED_REACH,     // Reach target within time bound
        TIME_BOUNDED_SAFETY     // Avoid target until time bound
    };

    /**
     * @brief Construct a reachability objective
     * 
     * @param type Type of objective
     * @param target_vertices Set of target vertices
     * @param time_bound Optional time bound (for time-bounded objectives)
     */
    ReachabilityObjective(ObjectiveType type, 
                         const std::set<GGGTemporalVertex>& target_vertices,
                         int time_bound = -1);

    /**
     * @brief Add a target vertex to the objective
     * 
     * @param vertex Target vertex to add
     */
    void add_target(GGGTemporalVertex vertex);

    /**
     * @brief Check if a vertex is a target
     * 
     * @param vertex Vertex to check
     * @return true if vertex is a target
     */
    bool is_target(GGGTemporalVertex vertex) const;

    /**
     * @brief Check if the objective is satisfied at current state
     * 
     * @param current_vertex Current vertex
     * @param current_time Current time
     * @return true if objective is satisfied
     */
    bool is_satisfied(GGGTemporalVertex current_vertex, int current_time) const;

    /**
     * @brief Check if the objective has failed at current state
     * 
     * @param current_vertex Current vertex  
     * @param current_time Current time
     * @return true if objective has failed
     */
    bool has_failed(GGGTemporalVertex current_vertex, int current_time) const;

    /**
     * @brief Get the objective type
     */
    ObjectiveType get_type() const { return type_; }

    /**
     * @brief Get target vertices
     */
    const std::set<GGGTemporalVertex>& get_targets() const { return targets_; }

    /**
     * @brief Get time bound (-1 if no bound)
     */
    int get_time_bound() const { return time_bound_; }

    /**
     * @brief Get string representation of objective
     */
    std::string to_string() const;

private:
    ObjectiveType type_;
    std::set<GGGTemporalVertex> targets_;
    int time_bound_;
};
