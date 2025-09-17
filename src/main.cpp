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
        } else if (variable_.empty()) {
            PresburgerTerm result = other;
            result.constant_ += constant_;
            return result;
        } else if (other.variable_.empty()) {
            PresburgerTerm result = *this;
            result.constant_ += other.constant_;
            return result;
        } else if (variable_ == other.variable_) {
            PresburgerTerm result = *this;
            result.coefficient_ += other.coefficient_;
            result.constant_ += other.constant_;
            return result;
        }
        // For different variables, return the first one (simplified)
        return *this;
    }
    
    PresburgerTerm operator*(int scalar) const {
        PresburgerTerm result = *this;
        result.coefficient_ *= scalar;
        result.constant_ *= scalar;
        return result;
    }
    
    std::string to_string() const {
        if (variable_.empty()) {
            return std::to_string(constant_);
        }
        
        std::string result;
        if (coefficient_ == 1) {
            result = variable_;
        } else if (coefficient_ == -1) {
            result = "-" + variable_;
        } else {
            result = std::to_string(coefficient_) + "*" + variable_;
        }
        
        if (constant_ > 0) {
            result += " + " + std::to_string(constant_);
        } else if (constant_ < 0) {
            result += " - " + std::to_string(-constant_);
        }
        
        return result;
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
        values["time"] = time;
        
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
        std::cout << "  time = current time\n\n";
        
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
 * @brief DOT file parser for Presburger temporal games
 */
class PresburgerTemporalDotParser {
private:
    std::map<std::string, PresburgerTemporalVertex> vertex_map_;

    std::unique_ptr<PresburgerFormula> parse_constraint(const std::string& constraint_str) {
        // Parse simple constraints like "time >= 2", "time = 3", "time <= 5"
        std::regex ge_pattern(R"(time\s*>=\s*(\d+))");
        std::regex eq_pattern(R"(time\s*=\s*(\d+))");
        std::regex le_pattern(R"(time\s*<=\s*(\d+))");
        
        // Parse existential constraints like "exists k. time = 2*k + 1"
        std::regex exists_pattern(R"(exists\s+(\w+)\.\s*(.+))");
        
        // Parse arithmetic constraints with variables like "time = 2*k + 1", "time >= k + 3"
        std::regex arith_eq_pattern(R"(time\s*=\s*(\d+)\*(\w+)\s*\+\s*(\d+))");
        std::regex arith_ge_pattern(R"(time\s*>=\s*(\w+)\s*\+\s*(\d+))");
        std::regex arith_le_pattern(R"(time\s*<=\s*(\w+)\s*\+\s*(\d+))");
        std::regex var_eq_pattern(R"(time\s*=\s*(\w+)\s*\+\s*(\d+))");
        
        std::smatch match;
        
        // Handle existential quantifiers
        if (std::regex_match(constraint_str, match, exists_pattern)) {
            std::string var = match[1].str();
            std::string inner_constraint = match[2].str();
            
            // Parse the inner constraint
            auto inner_formula = parse_constraint(inner_constraint);
            if (inner_formula) {
                return PresburgerFormula::exists(var, std::move(inner_formula));
            }
        }
        // Handle arithmetic expressions like "t = 2*k + 1"
        else if (std::regex_match(constraint_str, match, arith_eq_pattern)) {
            int coeff = std::stoi(match[1].str());
            std::string var = match[2].str();
            int constant = std::stoi(match[3].str());
            
            // Create term: coeff*var + constant
            PresburgerTerm right_term(var);
            right_term = right_term * coeff;
            PresburgerTerm const_term(constant);
            PresburgerTerm combined = right_term + const_term;
            
            return PresburgerFormula::equal(PresburgerTerm("time"), combined);
        }
        // Handle expressions like "time >= k + 3"
        else if (std::regex_match(constraint_str, match, arith_ge_pattern)) {
            std::string var = match[1].str();
            int constant = std::stoi(match[2].str());
            
            PresburgerTerm right_term(var);
            PresburgerTerm const_term(constant);
            PresburgerTerm combined = right_term + const_term;
            
            return PresburgerFormula::greaterequal(PresburgerTerm("time"), combined);
        }
        // Handle expressions like "time <= k + 5"
        else if (std::regex_match(constraint_str, match, arith_le_pattern)) {
            std::string var = match[1].str();
            int constant = std::stoi(match[2].str());
            
            PresburgerTerm right_term(var);
            PresburgerTerm const_term(constant);
            PresburgerTerm combined = right_term + const_term;
            
            return PresburgerFormula::lessequal(PresburgerTerm("time"), combined);
        }
        // Handle expressions like "time = k + 2"
        else if (std::regex_match(constraint_str, match, var_eq_pattern)) {
            std::string var = match[1].str();
            int constant = std::stoi(match[2].str());
            
            PresburgerTerm right_term(var);
            PresburgerTerm const_term(constant);
            PresburgerTerm combined = right_term + const_term;
            
            return PresburgerFormula::equal(PresburgerTerm("time"), combined);
        }
        // Handle simple constraints
        else if (std::regex_match(constraint_str, match, ge_pattern)) {
            int value = std::stoi(match[1].str());
            return PresburgerFormula::greaterequal(PresburgerTerm("time"), PresburgerTerm(value));
        } else if (std::regex_match(constraint_str, match, eq_pattern)) {
            int value = std::stoi(match[1].str());
            return PresburgerFormula::equal(PresburgerTerm("time"), PresburgerTerm(value));
        } else if (std::regex_match(constraint_str, match, le_pattern)) {
            int value = std::stoi(match[1].str());
            return PresburgerFormula::lessequal(PresburgerTerm("time"), PresburgerTerm(value));
        }
        
        return nullptr; // No constraint
    }

public:
    bool parse_file(const std::string& filename, PresburgerTemporalGameManager& manager) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return false;
        }
        
        std::string line;
        std::regex vertex_pattern(R"(\s*(\w+)\s*\[\s*name\s*=\s*\"([^\"]+)\"\s*,\s*player\s*=\s*(\d+)\s*\]\s*;)");
        std::regex edge_pattern(R"(\s*(\w+)\s*->\s*(\w+)\s*\[\s*label\s*=\s*\"([^\"]+)\"(?:\s*,\s*constraint\s*=\s*\"([^\"]+)\")?\s*\]\s*;?)");
        
        while (std::getline(file, line)) {
            // Skip comments and empty lines
            if (line.empty() || line.find("//") == 0 || line.find("digraph") != std::string::npos || 
                line.find("{") != std::string::npos || line.find("}") != std::string::npos) {
                continue;
            }
            
            std::smatch match;
            
            // Parse vertex declarations
            if (std::regex_search(line, match, vertex_pattern)) {
                std::string vertex_id = match[1].str();
                std::string name = match[2].str();
                int player = std::stoi(match[3].str());
                
                auto vertex = manager.add_vertex(name, player);
                vertex_map_[vertex_id] = vertex;
                continue;
            }
            
            // Parse edge declarations
            if (std::regex_search(line, match, edge_pattern)) {
                std::string source_id = match[1].str();
                std::string target_id = match[2].str();
                std::string label = match[3].str();
                std::string constraint_str = match[4].str(); // May be empty
                
                if (vertex_map_.find(source_id) == vertex_map_.end() || 
                    vertex_map_.find(target_id) == vertex_map_.end()) {
                    std::cerr << "Error: Unknown vertex in edge: " << source_id << " -> " << target_id << std::endl;
                    continue;
                }
                
                auto source = vertex_map_[source_id];
                auto target = vertex_map_[target_id];
                auto edge = manager.add_edge(source, target, label);
                
                // Parse and add constraint if present
                if (!constraint_str.empty()) {
                    auto constraint = parse_constraint(constraint_str);
                    if (constraint) {
                        manager.add_edge_constraint(edge, std::move(constraint));
                    }
                }
            }
        }
        
        file.close();
        return true;
    }
};

} // namespace graphs
} // namespace ggg

int main(int argc, char* argv[]) {
    using namespace ggg::graphs;
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <dot_file>" << std::endl;
        std::cerr << "Example: " << argv[0] << " example_temporal.dot" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
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