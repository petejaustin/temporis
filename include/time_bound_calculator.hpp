#pragma once

#include "temporal_game_manager.hpp"
#include "reachability_objective.hpp"
#include <algorithm>
#include <cmath>

namespace ggg {
namespace temporal {

/**
 * @brief Calculates appropriate time bounds for temporal reachability games
 * 
 * Provides research-based dynamic calculation of time bounds based on:
 * - Game structure (vertices, edges)
 * - Constraint complexity 
 * - Objective type
 * - User preferences
 */
class TimeBoundCalculator {
public:
    /**
     * @brief Configuration for time bound calculation
     */
    struct Config {
        int min_bound = 10;           // Minimum time bound
        int max_bound = 1000;         // Maximum time bound  
        double structure_factor = 2.0; // Multiplier for graph structure
        double constraint_factor = 1.5; // Multiplier for constraint complexity
        int user_override = -1;       // User-specified override (-1 = auto)
        bool verbose = false;         // Print calculation details
    };

    /**
     * @brief Construct calculator with configuration
     */
    TimeBoundCalculator();
    explicit TimeBoundCalculator(const Config& config);

    /**
     * @brief Calculate solver time bound for reachability games
     * 
     * @param manager Game manager containing graph structure
     * @param objective Reachability objective (affects bound calculation)
     * @return Recommended time bound for solver
     */
    int calculate_solver_bound(const graphs::PresburgerTemporalGameManager& manager,
                              const ReachabilityObjective& objective) const;

    /**
     * @brief Calculate analysis time window for temporal edge analysis
     * 
     * @param manager Game manager containing graph structure
     * @return Recommended time window for analysis
     */
    int calculate_analysis_window(const graphs::PresburgerTemporalGameManager& manager) const;

    /**
     * @brief Get detailed explanation of bound calculation
     * 
     * @param manager Game manager
     * @param objective Reachability objective
     * @return String explaining how bounds were calculated
     */
    std::string explain_calculation(const graphs::PresburgerTemporalGameManager& manager,
                                   const ReachabilityObjective& objective) const;

private:
    Config config_;

    /**
     * @brief Calculate base bound from graph structure
     */
    int calculate_structure_bound(const graphs::PresburgerTemporalGameManager& manager) const;

    /**
     * @brief Estimate constraint complexity factor
     */
    double estimate_constraint_complexity(const graphs::PresburgerTemporalGameManager& manager) const;

    /**
     * @brief Apply objective-specific adjustments
     */
    double get_objective_factor(const ReachabilityObjective& objective) const;

    /**
     * @brief Clamp bound to configured min/max values
     */
    int clamp_bound(int bound) const;
};

} // namespace temporal
} // namespace ggg
