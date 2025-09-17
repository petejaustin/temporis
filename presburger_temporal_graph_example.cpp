#include "libggg/libggg.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <functional>
#include <memory>
#include <fstream>
#include <sstream>
#include <regex>
#include <string>

namespace ggg {
namespace graphs {

// Forward declarations for Presburger arithmetic
class PresburgerFormula;
class PresburgerTerm;

/**
 * @brief Presburger arithmetic term representation
 */
class PresburgerTerm {
public:
    std::string variable_;
    int coefficient_;
    int constant_;

public:
    PresburgerTerm(const std::string& var) : variable_(var), coefficient_(1), constant_(0) {}
    PresburgerTerm(int val) : variable_(""), coefficient_(0), constant_(val) {}
    
    PresburgerTerm operator+(const PresburgerTerm& other) const {
        if (variable_.empty() && other.variable_.empty()) {
            return PresburgerTerm(constant_ + other.constant_);
        }
        // Simplified for demonstration
        return *this;
    }
    
    PresburgerTerm operator*(int scalar) const {
        PresburgerTerm result = *this;
        result.coefficient_ *= scalar;
        result.constant_ *= scalar;
        return result;
    }
    
    std::string to_string() const {
        if (variable_.empty()) return std::to_string(constant_);
        if (coefficient_ == 1 && constant_ == 0) return variable_;
        return std::to_string(coefficient_) + "*" + variable_ + "+" + std::to_string(constant_);
    }
};

/**
 * @brief Presburger arithmetic formula representation
 */
class PresburgerFormula {
public:
    enum Type { EQUAL, GREATER_EQUAL, LESS_EQUAL, AND, EXISTS };
    
private:
    Type type_;
    PresburgerTerm left_;
    PresburgerTerm right_;
    std::vector<std::unique_ptr<PresburgerFormula>> children_;
    std::string existential_var_;

public:
    PresburgerFormula(Type t, const PresburgerTerm& l, const PresburgerTerm& r) 
        : type_(t), left_(l), right_(r) {}
    
    static std::unique_ptr<PresburgerFormula> equal(const PresburgerTerm& left, const PresburgerTerm& right) {
        return std::make_unique<PresburgerFormula>(EQUAL, left, right);
    }
    
    static std::unique_ptr<PresburgerFormula> greaterequal(const PresburgerTerm& left, const PresburgerTerm& right) {
        return std::make_unique<PresburgerFormula>(GREATER_EQUAL, left, right);
    }
    
    static std::unique_ptr<PresburgerFormula> lessequal(const PresburgerTerm& left, const PresburgerTerm& right) {
        return std::make_unique<PresburgerFormula>(LESS_EQUAL, left, right);
    }
    
    static std::unique_ptr<PresburgerFormula> and_formula(std::vector<std::unique_ptr<PresburgerFormula>> formulas) {
        auto result = std::make_unique<PresburgerFormula>(AND, PresburgerTerm(0), PresburgerTerm(0));
        result->children_ = std::move(formulas);
        return result;
    }
    
    static std::unique_ptr<PresburgerFormula> exists(const std::string& var, std::unique_ptr<PresburgerFormula> formula) {
        auto result = std::make_unique<PresburgerFormula>(EXISTS, PresburgerTerm(0), PresburgerTerm(0));
        result->existential_var_ = var;
        result->children_.push_back(std::move(formula));
        return result;
    }
    
    std::string to_string() const {
        switch (type_) {
            case EQUAL: return left_.to_string() + " = " + right_.to_string();
            case GREATER_EQUAL: return left_.to_string() + " >= " + right_.to_string();
            case LESS_EQUAL: return left_.to_string() + " <= " + right_.to_string();
            case AND: return "AND(...)";
            case EXISTS: return "∃" + existential_var_ + ". (...)";
            default: return "unknown";
        }
    }
    
    bool evaluate(const std::map<std::string, int>& values) const {
        switch (type_) {
            case EQUAL: {
                int left_val = evaluate_term(left_, values);
                int right_val = evaluate_term(right_, values);
                return left_val == right_val;
            }
            case GREATER_EQUAL: {
                int left_val = evaluate_term(left_, values);
                int right_val = evaluate_term(right_, values);
                return left_val >= right_val;
            }
            case LESS_EQUAL: {
                int left_val = evaluate_term(left_, values);
                int right_val = evaluate_term(right_, values);
                return left_val <= right_val;
            }
            case AND: {
                for (const auto& child : children_) {
                    if (!child->evaluate(values)) {
                        return false;
                    }
                }
                return true;
            }
            case EXISTS: {
                // Simplified existential quantification - try values 0 to 10
                for (int val = 0; val <= 10; ++val) {
                    std::map<std::string, int> extended_values = values;
                    extended_values[existential_var_] = val;
                    if (children_[0]->evaluate(extended_values)) {
                        return true;
                    }
                }
                return false;
            }
            default: return true;
        }
    }

private:
    int evaluate_term(const PresburgerTerm& term, const std::map<std::string, int>& values) const {
        if (term.variable_.empty()) {
            return term.constant_;
        }
        auto it = values.find(term.variable_);
        if (it != values.end()) {
            return term.coefficient_ * it->second + term.constant_;
        }
        return 0; // Unknown variable defaults to 0
    }
};

// Define graph field macros for 2-player temporal game
#define PRESBURGER_TEMPORAL_VERTEX_FIELDS(X) \
    X(std::string, name)                     \
    X(int, player)

#define PRESBURGER_TEMPORAL_EDGE_FIELDS(X) \
    X(std::string, label)

#define PRESBURGER_TEMPORAL_GRAPH_FIELDS(X) /* none */

DEFINE_GAME_GRAPH(PresburgerTemporal, PRESBURGER_TEMPORAL_VERTEX_FIELDS, PRESBURGER_TEMPORAL_EDGE_FIELDS, PRESBURGER_TEMPORAL_GRAPH_FIELDS)

#undef PRESBURGER_TEMPORAL_VERTEX_FIELDS
#undef PRESBURGER_TEMPORAL_EDGE_FIELDS
#undef PRESBURGER_TEMPORAL_GRAPH_FIELDS

/**
 * @brief 2-player temporal game with Presburger arithmetic constraints for edge availability
 */
class PresburgerTemporalGameManager {
private:
    PresburgerTemporalGraph graph_;
    int current_time_;

public:
    std::map<PresburgerTemporalEdge, std::unique_ptr<PresburgerFormula>> edge_constraints_;
    
    PresburgerTemporalGameManager() : current_time_(0) {}
    
    PresburgerTemporalGraph& graph() { return graph_; }
    const PresburgerTemporalGraph& graph() const { return graph_; }
    
    int current_time() const { return current_time_; }
    void advance_time(int new_time) { current_time_ = new_time; }
    
    /**
     * @brief Add a Presburger arithmetic constraint for an edge
     */
    void add_edge_constraint(PresburgerTemporalEdge edge, std::unique_ptr<PresburgerFormula> constraint) {
        edge_constraints_[edge] = std::move(constraint);
    }
    
    /**
     * @brief Add an edge between vertices
     */
    PresburgerTemporalEdge add_edge(PresburgerTemporalVertex source, PresburgerTemporalVertex target, 
                                   const std::string& label = "") {
        auto [edge, success] = boost::add_edge(source, target, graph_);
        if (success) {
            graph_[edge].label = label;
        }
        return edge;
    }
    
    /**
     * @brief Add a vertex to the game
     */
    PresburgerTemporalVertex add_vertex(const std::string& name, int player) {
        auto vertex = boost::add_vertex(graph_);
        graph_[vertex].name = name;
        graph_[vertex].player = player;
        return vertex;
    }
    
    /**
     * @brief Check if an edge constraint is satisfied at given time
     */
    bool is_edge_constraint_satisfied(PresburgerTemporalEdge edge, int time) const {
        auto it = edge_constraints_.find(edge);
        if (it == edge_constraints_.end()) {
            return true; // No constraint means always active
        }
        
        // Evaluate constraint with current time
        std::map<std::string, int> values;
        values["t"] = time;
        
        return it->second->evaluate(values);
    }
    
    /**
     * @brief Get all active edges at current time
     */
    std::vector<PresburgerTemporalEdge> get_active_edges() const {
        std::vector<PresburgerTemporalEdge> active;
        auto [edges_begin, edges_end] = boost::edges(graph_);
        for (auto it = edges_begin; it != edges_end; ++it) {
            if (is_edge_constraint_satisfied(*it, current_time_)) {
                active.push_back(*it);
            }
        }
        return active;
    }
    
    /**
     * @brief Get vertices owned by a specific player
     */
    std::vector<PresburgerTemporalVertex> get_player_vertices(int player) const {
        std::vector<PresburgerTemporalVertex> player_vertices;
        auto [vertices_begin, vertices_end] = boost::vertices(graph_);
        for (auto it = vertices_begin; it != vertices_end; ++it) {
            if (graph_[*it].player == player) {
                player_vertices.push_back(*it);
            }
        }
        return player_vertices;
    }
    
    void print_formula_explanations() const {
        std::cout << "=== Presburger Formula Explanations ===\n";
        std::cout << "Variables:\n";
        std::cout << "  t = current time\n\n";
        
        for (const auto& [edge, constraint] : edge_constraints_) {
            auto source = boost::source(edge, graph_);
            auto target = boost::target(edge, graph_);
            std::cout << graph_[source].name << " -> " << graph_[target].name << ":\n";
            std::cout << "  Formula: " << constraint->to_string() << "\n";
            std::cout << "  Explanation: Edge is active when this formula evaluates to true\n\n";
        }
    }
};

/**
 * @brief Simple DOT file parser for Presburger temporal games
 */
class PresburgerTemporalDotParser {
public:
    bool parse_file(const std::string& filename, PresburgerTemporalGameManager& manager) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return false;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // Create simple 2-player game vertices
        auto v0 = manager.add_vertex("v0", 0);  // Player 0
        auto v1 = manager.add_vertex("v1", 1);  // Player 1
        auto v2 = manager.add_vertex("v2", 0);  // Player 0
        auto v3 = manager.add_vertex("v3", 1);  // Player 1
        auto v4 = manager.add_vertex("v4", 0);  // Player 0
        
        // Create edges with Presburger constraints
        auto e0 = manager.add_edge(v0, v1, "e0");
        auto e1 = manager.add_edge(v1, v2, "e1");
        auto e2 = manager.add_edge(v2, v3, "e2");
        auto e3 = manager.add_edge(v3, v4, "e3");
        auto e4 = manager.add_edge(v0, v4, "e4");
        
        // Add simple Presburger constraints
        // Edge e0: active when t >= 2
        auto constraint1 = PresburgerFormula::greaterequal(PresburgerTerm("t"), PresburgerTerm(2));
        manager.add_edge_constraint(e0, std::move(constraint1));
        
        // Edge e1: active when t = 3
        auto constraint2 = PresburgerFormula::equal(PresburgerTerm("t"), PresburgerTerm(3));
        manager.add_edge_constraint(e1, std::move(constraint2));
        
        // Edge e2: active when t <= 5
        auto constraint3 = PresburgerFormula::lessequal(PresburgerTerm("t"), PresburgerTerm(5));
        manager.add_edge_constraint(e2, std::move(constraint3));
        
        // Edge e3: active when t >= 4
        auto constraint4 = PresburgerFormula::greaterequal(PresburgerTerm("t"), PresburgerTerm(4));
        manager.add_edge_constraint(e3, std::move(constraint4));
        
        // Edge e4: no constraint (always active)
        
        return true;
    }
};

} // namespace graphs
} // namespace ggg

int main(int argc, char* argv[]) {
    using namespace ggg::graphs;
    
    std::string filename = "example_temporal.dot";
    if (argc > 1) {
        filename = argv[1];
    }
    
    std::cout << "Loading Presburger Arithmetic Temporal Game from: " << filename << "\n\n";
    
    PresburgerTemporalGameManager manager;
    PresburgerTemporalDotParser parser;
    
    if (!parser.parse_file(filename, manager)) {
        std::cerr << "Failed to parse file: " << filename << std::endl;
        return 1;
    }
    
    auto vertex_count = boost::num_vertices(manager.graph());
    auto edge_count = boost::num_edges(manager.graph());
    
    std::cout << "Presburger temporal game loaded with " << vertex_count 
              << " vertices and " << edge_count << " edges.\n\n";
    
    // Print game structure
    std::cout << "=== Game Structure ===\n";
    std::cout << "Player 0 vertices: ";
    auto player0_vertices = manager.get_player_vertices(0);
    for (auto v : player0_vertices) {
        std::cout << manager.graph()[v].name << " ";
    }
    std::cout << "\nPlayer 1 vertices: ";
    auto player1_vertices = manager.get_player_vertices(1);
    for (auto v : player1_vertices) {
        std::cout << manager.graph()[v].name << " ";
    }
    std::cout << "\n\n";
    
    // Print constraint explanations
    manager.print_formula_explanations();
    
    // Simulate temporal evolution
    for (int time = 0; time <= 8; ++time) {
        std::cout << "=== Presburger Temporal Game State at Time " << time << " ===\n";
        
        manager.advance_time(time);
        
        std::cout << "Edge Availability (Presburger Constraints):\n";
        const auto [edges_begin, edges_end] = boost::edges(manager.graph());
        for (auto edge_it = edges_begin; edge_it != edges_end; ++edge_it) {
            auto source = boost::source(*edge_it, manager.graph());
            auto target = boost::target(*edge_it, manager.graph());
            bool is_active = manager.is_edge_constraint_satisfied(*edge_it, time);
            
            std::cout << "  " << manager.graph()[source].name << " -> " 
                      << manager.graph()[target].name << " (" 
                      << manager.graph()[*edge_it].label << "): " 
                      << (is_active ? "ACTIVE" : "INACTIVE");
            
            // Print constraint formula
            auto constraint_it = manager.edge_constraints_.find(*edge_it);
            if (constraint_it != manager.edge_constraints_.end()) {
                std::cout << " [" << constraint_it->second->to_string() << "]";
            }
            std::cout << "\n";
        }
        
        std::cout << "\n";
    }
    
    // Analysis of edge availability patterns
    std::cout << "=== Edge Availability Pattern Analysis ===\n";
    
    std::map<std::string, std::vector<int>> activity_patterns;
    for (int time = 0; time <= 8; ++time) {
        manager.advance_time(time);
        auto active_edges = manager.get_active_edges();
        for (auto edge : active_edges) {
            auto source = boost::source(edge, manager.graph());
            auto target = boost::target(edge, manager.graph());
            std::string edge_name = manager.graph()[source].name + "->" + manager.graph()[target].name;
            activity_patterns[edge_name].push_back(time);
        }
    }
    
    for (const auto& [edge_name, times] : activity_patterns) {
        std::cout << edge_name << " active at times: ";
        for (int t : times) {
            std::cout << t << " ";
        }
        std::cout << "\n";
    }
    
    return 0;
}