#include "ggg_temporal_graph.hpp"
#include "presburger_formula.hpp"
#include "dot_parser.hpp"
#include "temporal_game_manager.hpp"  // For conversion from old format
#include <boost/graph/graph_traits.hpp>
#include <iostream>

namespace ggg {
namespace graphs {

GGGTemporalGameManager::GGGTemporalGameManager() 
    : graph_(std::make_shared<GGGTemporalGraph>()), current_time_(0) {
}

GGGTemporalVertex GGGTemporalGameManager::add_vertex(const std::string& name, int player, int target) {
    return ggg::graphs::add_vertex(*graph_, name, player, target);
}

std::pair<GGGTemporalEdge, bool> GGGTemporalGameManager::add_edge(
    GGGTemporalVertex source, GGGTemporalVertex target, const std::string& label) {
    return ggg::graphs::add_edge(*graph_, source, target, label);
}

void GGGTemporalGameManager::add_edge_constraint(GGGTemporalEdge edge, 
                                                std::unique_ptr<PresburgerFormula> constraint) {
    edge_constraints_[edge] = std::move(constraint);
}

bool GGGTemporalGameManager::is_edge_constraint_satisfied(GGGTemporalEdge edge, int time) const {
    auto it = edge_constraints_.find(edge);
    if (it == edge_constraints_.end()) {
        return true; // No constraint means always available
    }
    
    try {
        std::map<std::string, int> variables = {{"time", time}};
        return it->second->evaluate(variables);
    } catch (const std::exception&) {
        return false; // If evaluation fails, edge is not available
    }
}

void GGGTemporalGameManager::advance_time(int new_time) {
    current_time_ = new_time;
}

int GGGTemporalGameManager::current_time() const {
    return current_time_;
}

void GGGTemporalGameManager::clear_graph() {
    graph_ = std::make_shared<GGGTemporalGraph>();
    edge_constraints_.clear();
    current_time_ = 0;
}

std::vector<GGGTemporalVertex> GGGTemporalGameManager::get_available_moves(
    GGGTemporalVertex vertex, int time) const {
    std::vector<GGGTemporalVertex> moves;
    
    auto [edge_begin, edge_end] = boost::out_edges(vertex, *graph_);
    for (auto edge_it = edge_begin; edge_it != edge_end; ++edge_it) {
        if (is_edge_constraint_satisfied(*edge_it, time)) {
            moves.push_back(boost::target(*edge_it, *graph_));
        }
    }
    
    return moves;
}

std::set<GGGTemporalVertex> GGGTemporalGameManager::get_target_vertices() const {
    std::set<GGGTemporalVertex> targets;
    
    auto [vertex_begin, vertex_end] = boost::vertices(*graph_);
    for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
        if ((*graph_)[*vertex_it].target == 1) {
            targets.insert(*vertex_it);
        }
    }
    
    return targets;
}

bool GGGTemporalGameManager::load_from_dot_file(const std::string& filename) {
    // Delegate to existing DOT parser
    PresburgerTemporalDotParser parser;
    auto old_manager = std::make_unique<::PresburgerTemporalGameManager>(); // Use old namespace
    
    std::shared_ptr<::ReachabilityObjective> objective;
    bool success = parser.parse_file_with_objective(filename, *old_manager, objective);
    
    if (!success) {
        return false;
    }
    
    // Convert from old manager to new GGG-style manager
    clear_graph();
    
    // Copy vertices
    std::map<::PresburgerTemporalVertex, GGGTemporalVertex> vertex_mapping;
    auto [old_vertex_begin, old_vertex_end] = boost::vertices(old_manager->graph());
    
    for (auto old_vertex_it = old_vertex_begin; old_vertex_it != old_vertex_end; ++old_vertex_it) {
        const auto& old_props = old_manager->graph()[*old_vertex_it];
        int target = 0; // Will be updated from objective
        
        GGGTemporalVertex new_vertex = add_vertex(old_props.name, old_props.player, target);
        vertex_mapping[*old_vertex_it] = new_vertex;
    }
    
    // Update target vertices if objective was parsed
    if (objective) {
        auto targets = objective->get_targets();
        for (auto old_target : targets) {
            if (vertex_mapping.find(old_target) != vertex_mapping.end()) {
                auto new_target = vertex_mapping[old_target];
                (*graph_)[new_target].target = 1;
            }
        }
    }
    
    // Copy edges and constraints
    auto [old_edge_begin, old_edge_end] = boost::edges(old_manager->graph());
    for (auto old_edge_it = old_edge_begin; old_edge_it != old_edge_end; ++old_edge_it) {
        auto old_source = boost::source(*old_edge_it, old_manager->graph());
        auto old_target = boost::target(*old_edge_it, old_manager->graph());
        
        auto new_source = vertex_mapping[old_source];
        auto new_target_vertex = vertex_mapping[old_target];
        
        const auto& old_edge_props = old_manager->graph()[*old_edge_it];
        auto [new_edge, success] = add_edge(new_source, new_target_vertex, old_edge_props.label);
        
        // Copy edge constraints if they exist
        // TODO: Add public accessor method to get edge constraints from old manager
        if (success) {
            // For now, skip constraint copying due to private member access
            // This will be addressed in the detailed migration step
        }
    }
    
    return true;
}

bool GGGTemporalGameManager::validate_game_structure() const {
    // Use GGG graph structure validation where possible
    if (boost::num_vertices(*graph_) == 0) {
        return false;
    }
    
    // Check that each vertex has at least one outgoing edge
    auto [vertex_begin, vertex_end] = boost::vertices(*graph_);
    for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
        if (boost::out_degree(*vertex_it, *graph_) == 0) {
            return false;
        }
    }
    
    // Check that at least one vertex is a target
    return !get_target_vertices().empty();
}

// GGGReachabilityObjective implementation
GGGReachabilityObjective::GGGReachabilityObjective(Type type, 
                                                   const std::set<GGGTemporalVertex>& targets,
                                                   int time_bound)
    : type_(type), target_vertices_(targets), time_bound_(time_bound) {
}

bool GGGReachabilityObjective::is_target(GGGTemporalVertex vertex) const {
    return target_vertices_.find(vertex) != target_vertices_.end();
}

bool GGGReachabilityObjective::is_satisfied(GGGTemporalVertex vertex, int time) const {
    switch (type_) {
        case Type::REACHABILITY:
            return is_target(vertex);
        case Type::TIME_BOUNDED_REACH:
            return is_target(vertex) && (time_bound_ < 0 || time <= time_bound_);
        case Type::SAFETY:
            return !is_target(vertex);
        case Type::TIME_BOUNDED_SAFETY:
            return !is_target(vertex) || (time_bound_ >= 0 && time > time_bound_);
        default:
            return false;
    }
}

bool GGGReachabilityObjective::has_failed(GGGTemporalVertex vertex, int time) const {
    switch (type_) {
        case Type::REACHABILITY:
            return false; // Never fails permanently
        case Type::TIME_BOUNDED_REACH:
            return time_bound_ >= 0 && time > time_bound_ && !is_target(vertex);
        case Type::SAFETY:
            return is_target(vertex);
        case Type::TIME_BOUNDED_SAFETY:
            return is_target(vertex) && (time_bound_ < 0 || time <= time_bound_);
        default:
            return true;
    }
}

} // namespace graphs
} // namespace ggg
