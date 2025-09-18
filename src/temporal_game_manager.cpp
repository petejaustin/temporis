#include "temporal_game_manager.hpp"
#include <iostream>
#include <vector>
#include <algorithm>

namespace ggg {
namespace graphs {

PresburgerTemporalGameManager::PresburgerTemporalGameManager() : current_time_(0) {}

PresburgerTemporalVertex PresburgerTemporalGameManager::add_vertex(const std::string& name, int player) {
    auto vertex = boost::add_vertex(graph_);
    graph_[vertex].name = name;
    graph_[vertex].player = player;
    return vertex;
}

PresburgerTemporalEdge PresburgerTemporalGameManager::add_edge(PresburgerTemporalVertex source, PresburgerTemporalVertex target, const std::string& label) {
    auto [edge, success] = boost::add_edge(source, target, graph_);
    if (success) {
        graph_[edge].label = label;
    }
    return edge;
}

void PresburgerTemporalGameManager::add_edge_constraint(PresburgerTemporalEdge edge, std::unique_ptr<PresburgerFormula> constraint) {
    edge_constraints_[edge] = std::move(constraint);
}

PresburgerTemporalGraph& PresburgerTemporalGameManager::graph() {
    return graph_;
}

const PresburgerTemporalGraph& PresburgerTemporalGameManager::graph() const {
    return graph_;
}

void PresburgerTemporalGameManager::advance_time(int new_time) {
    current_time_ = new_time;
}

int PresburgerTemporalGameManager::current_time() const {
    return current_time_;
}

bool PresburgerTemporalGameManager::is_edge_constraint_satisfied(PresburgerTemporalEdge edge, int time) const {
    auto it = edge_constraints_.find(edge);
    if (it == edge_constraints_.end()) {
        return true; // No constraint means always active
    }
    
    // Evaluate constraint with current time
    std::map<std::string, int> values;
    values["time"] = time;
    
    return it->second->evaluate(values);
}

std::vector<PresburgerTemporalEdge> PresburgerTemporalGameManager::get_active_edges() const {
    std::vector<PresburgerTemporalEdge> active;
    auto [edges_begin, edges_end] = boost::edges(graph_);
    for (auto it = edges_begin; it != edges_end; ++it) {
        if (is_edge_constraint_satisfied(*it, current_time_)) {
            active.push_back(*it);
        }
    }
    return active;
}

std::vector<PresburgerTemporalVertex> PresburgerTemporalGameManager::get_player_vertices(int player) const {
    std::vector<PresburgerTemporalVertex> player_vertices;
    auto [vertices_begin, vertices_end] = boost::vertices(graph_);
    for (auto it = vertices_begin; it != vertices_end; ++it) {
        if (graph_[*it].player == player) {
            player_vertices.push_back(*it);
        }
    }
    return player_vertices;
}

void PresburgerTemporalGameManager::print_formula_explanations() const {
    std::cout << "=== Presburger Formula Explanations ===\n";
    std::cout << "Variables:\n";
    std::cout << "  time = current time\n\n";
    
    for (const auto& [edge, constraint] : edge_constraints_) {
        auto source = boost::source(edge, graph_);
        auto target = boost::target(edge, graph_);
        std::cout << graph_[source].name << " -> " << graph_[target].name << ":\n";
        std::cout << "  Formula: " << constraint->to_string() << "\n";
        std::cout << "  Explanation: Edge is active when this formula evaluates to true\n\n";
    }
}

} // namespace graphs
} // namespace ggg
