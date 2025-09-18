#include "reachability_objective.hpp"

ReachabilityObjective::ReachabilityObjective(ObjectiveType type,
                                           const std::set<PresburgerTemporalVertex>& target_vertices,
                                           int time_bound)
    : type_(type), targets_(target_vertices), time_bound_(time_bound) {
}

void ReachabilityObjective::add_target(PresburgerTemporalVertex vertex) {
    targets_.insert(vertex);
}

bool ReachabilityObjective::is_target(PresburgerTemporalVertex vertex) const {
    return targets_.find(vertex) != targets_.end();
}

bool ReachabilityObjective::is_satisfied(PresburgerTemporalVertex current_vertex, int current_time) const {
    bool at_target = is_target(current_vertex);
    
    switch (type_) {
        case ObjectiveType::REACHABILITY:
        case ObjectiveType::TIME_BOUNDED_REACH:
            return at_target;
            
        case ObjectiveType::SAFETY:
            return !at_target;
            
        case ObjectiveType::TIME_BOUNDED_SAFETY:
            if (time_bound_ >= 0 && current_time >= time_bound_) {
                return true; // Safety period completed
            }
            return !at_target;
    }
    return false;
}

bool ReachabilityObjective::has_failed(PresburgerTemporalVertex current_vertex, int current_time) const {
    bool at_target = is_target(current_vertex);
    
    switch (type_) {
        case ObjectiveType::REACHABILITY:
            return false; // Never fails, just hasn't succeeded yet
            
        case ObjectiveType::TIME_BOUNDED_REACH:
            if (time_bound_ >= 0 && current_time > time_bound_) {
                return !at_target; // Failed to reach in time
            }
            return false;
            
        case ObjectiveType::SAFETY:
            return at_target; // Failed if reached target
            
        case ObjectiveType::TIME_BOUNDED_SAFETY:
            return at_target; // Failed if reached target before time bound
    }
    return false;
}

std::string ReachabilityObjective::to_string() const {
    std::string result;
    
    switch (type_) {
        case ObjectiveType::REACHABILITY:
            result = "Reachability: Player 0 wins by reaching target";
            break;
        case ObjectiveType::SAFETY:
            result = "Safety: Player 0 wins by avoiding target";
            break;
        case ObjectiveType::TIME_BOUNDED_REACH:
            result = "Time-bounded Reachability: Player 0 wins by reaching target within " + std::to_string(time_bound_) + " time steps";
            break;
        case ObjectiveType::TIME_BOUNDED_SAFETY:
            result = "Time-bounded Safety: Player 0 wins by avoiding target for " + std::to_string(time_bound_) + " time steps";
            break;
    }
    
    result += "\nTarget vertices: ";
    for (auto target : targets_) {
        result += std::to_string(target) + " ";
    }
    
    return result;
}
