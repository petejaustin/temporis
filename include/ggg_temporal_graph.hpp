#pragma once
#include "libggg/graphs/graph_utilities.hpp"
#include "presburger_formula.hpp"
#include <memory>
#include <map>
#include <set>
#include <optional>

namespace ggg {
namespace graphs {

// --- Define property field lists as macros for GGGTemporal graph ---
#define GGG_TEMPORAL_VERTEX_FIELDS(X) \
    X(std::string, name)                      \
    X(int, player)                            \
    X(int, target)

#define GGG_TEMPORAL_EDGE_FIELDS(X) \
    X(std::string, label) \
    X(PresburgerFormula, constraint)

#define GGG_TEMPORAL_GRAPH_FIELDS(X) /* none */

// Define the graph type using GGG's macro system
DEFINE_GAME_GRAPH(GGGTemporal, GGG_TEMPORAL_VERTEX_FIELDS, GGG_TEMPORAL_EDGE_FIELDS, GGG_TEMPORAL_GRAPH_FIELDS)

#undef GGG_TEMPORAL_VERTEX_FIELDS
#undef GGG_TEMPORAL_EDGE_FIELDS
#undef GGG_TEMPORAL_GRAPH_FIELDS

// Type aliases for convenience
using GGGTemporalVertex = GGGTemporalGraph::vertex_descriptor;
using GGGTemporalEdge = GGGTemporalGraph::edge_descriptor;

/**
 * @brief Enhanced manager for GGG-style temporal games using GGG infrastructure
 * 
 * This class extends the basic GGG graph with temporal constraint management
 * while maintaining compatibility with the GGG solver framework.
 */
class GGGTemporalGameManager {
private:
    std::shared_ptr<GGGTemporalGraph> graph_;
    int current_time_;
    
    // Constraint parsing methods (adapted from PresburgerTemporalDotParser)
    std::shared_ptr<PresburgerFormula> parse_constraint(const std::string& constraint_str) const;
    std::shared_ptr<PresburgerFormula> parse_existential_formula(const std::string& formula_str) const;
    std::shared_ptr<PresburgerFormula> parse_comparison_formula(const std::string& formula_str, const std::string& op, size_t pos) const;
    std::shared_ptr<PresburgerFormula> parse_logical_formula(const std::string& formula_str, const std::string& op, size_t pos) const;
    std::shared_ptr<PresburgerFormula> parse_modulus_constraint(const std::string& formula_str, size_t mod_pos) const;
    std::shared_ptr<PresburgerFormula> parse_percent_modulus_constraint(const std::string& formula_str, size_t percent_pos) const;
    std::shared_ptr<PresburgerTerm> parse_presburger_term(const std::string& term_str) const;public:
    GGGTemporalGameManager();
    
    // Graph access methods
    std::shared_ptr<GGGTemporalGraph> graph() { return graph_; }
    const std::shared_ptr<GGGTemporalGraph> graph() const { return graph_; }
    
    // Vertex and edge management using GGG utilities
    GGGTemporalVertex add_vertex(const std::string& name, int player, int target = 0);
    std::pair<GGGTemporalEdge, bool> add_edge(GGGTemporalVertex source, 
                                              GGGTemporalVertex target, 
                                              const std::string& label = "",
                                              const PresburgerFormula& constraint = PresburgerFormula(PresburgerFormula::EQUAL, PresburgerTerm(1), PresburgerTerm(1)));
    
    // Temporal constraint management
    void set_edge_constraint(GGGTemporalEdge edge, const PresburgerFormula& constraint);
    bool is_edge_constraint_satisfied(GGGTemporalEdge edge, int time) const;
    
    // Time management
    void advance_time(int new_time);
    int current_time() const;
    
    // Utilities
    void clear_graph();
    
    // Game analysis methods
    std::vector<GGGTemporalVertex> get_available_moves(GGGTemporalVertex vertex, int time) const;
    std::set<GGGTemporalVertex> get_target_vertices() const;
    
    // Integration with existing parsers
    bool load_from_dot_file(const std::string& filename);
    bool validate_game_structure() const;
};

/**
 * @brief GGG-style reachability objective
 */
class GGGReachabilityObjective {
public:
    enum class Type {
        REACHABILITY,           // Reach target eventually
        SAFETY,                 // Avoid target forever  
        TIME_BOUNDED_REACH,     // Reach target within time bound
        TIME_BOUNDED_SAFETY     // Avoid target until time bound
    };

private:
    Type type_;
    std::set<GGGTemporalVertex> target_vertices_;
    int time_bound_;

public:
    GGGReachabilityObjective(Type type, 
                            const std::set<GGGTemporalVertex>& targets,
                            int time_bound = -1);
    
    bool is_target(GGGTemporalVertex vertex) const;
    bool is_satisfied(GGGTemporalVertex vertex, int time) const;
    bool has_failed(GGGTemporalVertex vertex, int time) const;
    
    Type get_type() const { return type_; }
    const std::set<GGGTemporalVertex>& get_targets() const { return target_vertices_; }
    int get_time_bound() const { return time_bound_; }
};

} // namespace graphs
} // namespace ggg
