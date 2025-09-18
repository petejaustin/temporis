#pragma once

#include "libggg/libggg.hpp"
#include "presburger_formula.hpp"
#include <memory>
#include <map>

namespace ggg {
namespace graphs {

// Define graph field macros for 2-player temporal game
#define PRESBURGER_TEMPORAL_VERTEX_FIELDS(X) \
    X(std::string, name)                     \
    X(int, player)

#define PRESBURGER_TEMPORAL_EDGE_FIELDS(X) \
    X(std::string, label)

#define PRESBURGER_TEMPORAL_GRAPH_FIELDS(X) /* none */

#define PRESBURGER_TEMPORAL_VERTEX_FIELDS(X) \
    X(std::string, name)                     \
    X(int, player)

#define PRESBURGER_TEMPORAL_EDGE_FIELDS(X) \
    X(std::string, label)

#define PRESBURGER_TEMPORAL_GRAPH_FIELDS(X) /* none */

DEFINE_GAME_GRAPH(PresburgerTemporal, PRESBURGER_TEMPORAL_VERTEX_FIELDS, PRESBURGER_TEMPORAL_EDGE_FIELDS, PRESBURGER_TEMPORAL_GRAPH_FIELDS);

#undef PRESBURGER_TEMPORAL_VERTEX_FIELDS
#undef PRESBURGER_TEMPORAL_EDGE_FIELDS
#undef PRESBURGER_TEMPORAL_GRAPH_FIELDS

typedef boost::graph_traits<PresburgerTemporalGraph>::vertex_descriptor PresburgerTemporalVertex;
typedef boost::graph_traits<PresburgerTemporalGraph>::edge_descriptor PresburgerTemporalEdge;

/**
 * @brief Manages Presburger arithmetic temporal games
 */
class PresburgerTemporalGameManager {
private:
    PresburgerTemporalGraph graph_;
    std::map<PresburgerTemporalEdge, std::unique_ptr<PresburgerFormula>> edge_constraints_;
    int current_time_;

public:
    PresburgerTemporalGameManager();
    
    PresburgerTemporalVertex add_vertex(const std::string& name, int player);
    PresburgerTemporalEdge add_edge(PresburgerTemporalVertex source, PresburgerTemporalVertex target, const std::string& label = "");
    void add_edge_constraint(PresburgerTemporalEdge edge, std::unique_ptr<PresburgerFormula> constraint);
    
    PresburgerTemporalGraph& graph();
    const PresburgerTemporalGraph& graph() const;
    void advance_time(int new_time);
    int current_time() const;
    
    bool is_edge_constraint_satisfied(PresburgerTemporalEdge edge, int time) const;
    std::vector<PresburgerTemporalEdge> get_active_edges() const;
    std::vector<PresburgerTemporalVertex> get_player_vertices(int player) const;
    
    void print_formula_explanations() const;
};

} // namespace graphs
} // namespace ggg
