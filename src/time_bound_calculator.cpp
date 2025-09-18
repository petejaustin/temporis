#include "time_bound_calculator.hpp"
#include <boost/graph/graph_traits.hpp>
#include <iostream>
#include <sstream>

namespace ggg {
namespace temporal {

TimeBoundCalculator::TimeBoundCalculator() : config_(Config{}) {}

TimeBoundCalculator::TimeBoundCalculator(const Config& config) : config_(config) {}

int TimeBoundCalculator::calculate_solver_bound(const graphs::PresburgerTemporalGameManager& manager,
                                               const ReachabilityObjective& objective) const {
    // User override takes precedence
    if (config_.user_override > 0) {
        if (config_.verbose) {
            std::cout << "Using user-specified time bound: " << config_.user_override << std::endl;
        }
        return config_.user_override;
    }

    // Calculate base bound from graph structure
    int structure_bound = calculate_structure_bound(manager);
    
    // Apply constraint complexity factor
    double constraint_complexity = estimate_constraint_complexity(manager);
    
    // Apply objective-specific factor
    double objective_factor = get_objective_factor(objective);
    
    // Combine factors
    double raw_bound = structure_bound * config_.structure_factor * 
                      constraint_complexity * config_.constraint_factor * 
                      objective_factor;
    
    int final_bound = clamp_bound(static_cast<int>(std::ceil(raw_bound)));
    
    if (config_.verbose) {
        std::cout << "Time bound calculation:\n"
                  << "  Structure bound: " << structure_bound << "\n"
                  << "  Constraint complexity: " << constraint_complexity << "\n"
                  << "  Objective factor: " << objective_factor << "\n"
                  << "  Raw calculation: " << raw_bound << "\n"
                  << "  Final bound: " << final_bound << std::endl;
    }
    
    return final_bound;
}

int TimeBoundCalculator::calculate_analysis_window(const graphs::PresburgerTemporalGameManager& manager) const {
    // Analysis window is typically smaller than solver bound
    // Focus on immediate temporal behavior
    int structure_bound = calculate_structure_bound(manager);
    double constraint_complexity = estimate_constraint_complexity(manager);
    
    // Use more conservative factors for analysis
    double raw_window = structure_bound * 1.5 * constraint_complexity;
    int final_window = clamp_bound(static_cast<int>(std::ceil(raw_window)));
    
    // Cap analysis window at reasonable size for output readability
    final_window = std::min(final_window, 50);
    
    if (config_.verbose) {
        std::cout << "Analysis window calculation:\n"
                  << "  Structure bound: " << structure_bound << "\n"
                  << "  Constraint complexity: " << constraint_complexity << "\n"
                  << "  Final window: " << final_window << std::endl;
    }
    
    return final_window;
}

std::string TimeBoundCalculator::explain_calculation(const graphs::PresburgerTemporalGameManager& manager,
                                                    const ReachabilityObjective& objective) const {
    std::ostringstream explanation;
    
    if (config_.user_override > 0) {
        explanation << "Time bound: " << config_.user_override << " (user-specified override)";
        return explanation.str();
    }
    
    int structure_bound = calculate_structure_bound(manager);
    double constraint_complexity = estimate_constraint_complexity(manager);
    double objective_factor = get_objective_factor(objective);
    int solver_bound = calculate_solver_bound(manager, objective);
    int analysis_window = calculate_analysis_window(manager);
    
    explanation << "Time Bound Calculation (Research-Based):\n"
                << "├─ Graph Structure:\n"
                << "│  ├─ Vertices: " << boost::num_vertices(manager.graph()) << "\n"
                << "│  ├─ Edges: " << boost::num_edges(manager.graph()) << "\n"
                << "│  └─ Structure bound: " << structure_bound << "\n"
                << "├─ Constraint Complexity: " << constraint_complexity << "x\n"
                << "├─ Objective Factor (" << (objective.get_type() == ReachabilityObjective::ObjectiveType::REACHABILITY ? "reach" : "other") << "): " << objective_factor << "x\n"
                << "├─ Configuration:\n"
                << "│  ├─ Structure factor: " << config_.structure_factor << "x\n"
                << "│  ├─ Constraint factor: " << config_.constraint_factor << "x\n"
                << "│  ├─ Min/Max bounds: [" << config_.min_bound << ", " << config_.max_bound << "]\n"
                << "├─ Results:\n"
                << "│  ├─ Solver bound: " << solver_bound << " steps\n"
                << "│  └─ Analysis window: " << analysis_window << " steps\n"
                << "└─ Basis: Temporal reachability game complexity theory";
    
    return explanation.str();
}

int TimeBoundCalculator::calculate_structure_bound(const graphs::PresburgerTemporalGameManager& manager) const {
    auto num_vertices = boost::num_vertices(manager.graph());
    auto num_edges = boost::num_edges(manager.graph());
    
    if (num_vertices == 0) return config_.min_bound;
    
    // Research-based formula: combines graph diameter with connectivity
    // For temporal games, bound should consider:
    // 1. Graph diameter (worst-case path length)
    // 2. Potential cycles and reconvergence
    // 3. Temporal constraint evaluation overhead
    
    int diameter_bound = std::max(2 * static_cast<int>(num_vertices), 10);
    int connectivity_bound = static_cast<int>(num_vertices + num_edges / std::max(1ul, num_vertices));
    
    return std::max(diameter_bound, connectivity_bound);
}

double TimeBoundCalculator::estimate_constraint_complexity(const graphs::PresburgerTemporalGameManager& manager) const {
    // TODO: Analyze actual Presburger formulas for complexity
    // For now, use conservative estimate based on edge count
    auto num_edges = boost::num_edges(manager.graph());
    
    if (num_edges == 0) return 1.0;
    
    // Assume more edges = more complex constraints
    // This is a heuristic that should be refined based on actual constraint parsing
    if (num_edges <= 5) return 1.0;
    if (num_edges <= 20) return 1.2;
    if (num_edges <= 50) return 1.5;
    return 2.0;
}

double TimeBoundCalculator::get_objective_factor(const ReachabilityObjective& objective) const {
    switch (objective.get_type()) {
        case ReachabilityObjective::ObjectiveType::REACHABILITY:
            return 1.0; // Standard reachability
        case ReachabilityObjective::ObjectiveType::SAFETY:
            return 1.5; // Safety requires longer verification
        case ReachabilityObjective::ObjectiveType::TIME_BOUNDED_REACH:
            // Use objective's time bound if available
            if (objective.get_time_bound() > 0) {
                return std::min(2.0, objective.get_time_bound() / 20.0);
            }
            return 1.2;
        case ReachabilityObjective::ObjectiveType::TIME_BOUNDED_SAFETY:
            return 1.3;
        default:
            return 1.0;
    }
}

int TimeBoundCalculator::clamp_bound(int bound) const {
    return std::max(config_.min_bound, std::min(config_.max_bound, bound));
}

} // namespace temporal
} // namespace ggg
