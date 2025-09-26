#include "ggg_temporal_graph.hpp"
#include "presburger_formula.hpp"
#include <boost/graph/graph_traits.hpp>
#include <iostream>
#include <fstream>
#include <regex>
#include <algorithm>

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
    // Native GGG DOT parser - parse directly into GGG graph types
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }
    
    clear_graph();
    
    std::string line;
    std::map<std::string, GGGTemporalVertex> vertex_map;
    
    // Regex patterns for parsing
    std::regex vertex_pattern(R"(\s*(\w+)\s*\[\s*name\s*=\s*\"([^\"]+)\"\s*,\s*player\s*=\s*(\d+)(?:\s*,\s*target\s*=\s*(\d+))?\s*\]\s*;)");
    std::regex edge_pattern(R"(\s*(\w+)\s*->\s*(\w+)(?:\s*\[\s*label\s*=\s*\"([^\"]*)\"\s*\])?\s*;)");
    std::regex constraint_pattern(R"(\s*(\w+)\s*->\s*(\w+)\s*\[\s*constraint\s*=\s*\"([^\"]+)\"\s*\]\s*;)");
    
    while (std::getline(file, line)) {
        std::smatch match;
        
        // Parse vertex definitions
        if (std::regex_search(line, match, vertex_pattern)) {
            std::string vertex_id = match[1].str();
            std::string vertex_name = match[2].str();
            int player = std::stoi(match[3].str());
            int target = (match.size() > 4 && !match[4].str().empty()) ? std::stoi(match[4].str()) : 0;
            
            GGGTemporalVertex vertex = add_vertex(vertex_name, player, target);
            vertex_map[vertex_id] = vertex;
        }
        // Parse edge definitions
        else if (std::regex_search(line, match, edge_pattern)) {
            std::string source_id = match[1].str();
            std::string target_id = match[2].str();
            std::string label = match.size() > 3 ? match[3].str() : "";
            
            if (vertex_map.find(source_id) != vertex_map.end() && 
                vertex_map.find(target_id) != vertex_map.end()) {
                add_edge(vertex_map[source_id], vertex_map[target_id], label);
            }
        }
        // Parse constraint edges (now with full constraint parsing)
        else if (std::regex_search(line, match, constraint_pattern)) {
            std::string source_id = match[1].str();
            std::string target_id = match[2].str();
            std::string constraint_str = match[3].str();
            
            if (vertex_map.find(source_id) != vertex_map.end() && 
                vertex_map.find(target_id) != vertex_map.end()) {
                // Add edge and parse constraint
                auto edge = add_edge(vertex_map[source_id], vertex_map[target_id]);
                auto constraint = parse_constraint(constraint_str);
                add_edge_constraint(edge.first, std::move(constraint));
            }
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

// Constraint parsing methods (adapted from PresburgerTemporalDotParser)
std::unique_ptr<PresburgerFormula> GGGTemporalGameManager::parse_constraint(const std::string& constraint_str) {
    // Remove whitespace
    std::string cleaned = constraint_str;
    cleaned.erase(std::remove_if(cleaned.begin(), cleaned.end(), ::isspace), cleaned.end());
    
    // Handle simple cases first
    if (cleaned == "true") {
        return PresburgerFormula::equal(
            PresburgerTerm(1), 
            PresburgerTerm(1)
        );
    }
    if (cleaned == "false") {
        return PresburgerFormula::equal(
            PresburgerTerm(1), 
            PresburgerTerm(0)
        );
    }
    
    // Parse existential quantifiers
    if (cleaned.starts_with("exists")) {
        return parse_existential_formula(cleaned);
    }
    
    // Parse negation operator
    if (cleaned.starts_with("!")) {
        std::string inner_formula = cleaned.substr(1);
        auto inner = parse_constraint(inner_formula);
        return PresburgerFormula::not_formula(std::move(inner));
    }
    
    // Parse parenthesized expressions
    if (cleaned.starts_with("(") && cleaned.ends_with(")")) {
        std::string inner_formula = cleaned.substr(1, cleaned.length() - 2);
        return parse_constraint(inner_formula);
    }
    
    // Parse modulus constraints: expr%m==r or expr mod m == r
    auto mod_pos = cleaned.find("mod");
    if (mod_pos != std::string::npos) {
        return parse_modulus_constraint(cleaned, mod_pos);
    }
    
    // Parse modulus with % operator: expr%m==r
    auto percent_pos = cleaned.find('%');
    if (percent_pos != std::string::npos) {
        return parse_percent_modulus_constraint(cleaned, percent_pos);
    }
    
    // Parse comparison operators
    for (const auto& op : {">=", "<=", ">", "<", "==", "!="}) {
        auto pos = cleaned.find(op);
        if (pos != std::string::npos) {
            return parse_comparison_formula(cleaned, op, pos);
        }
    }
    
    // Parse logical operators
    for (const auto& op : {"&&", "||"}) {
        auto pos = cleaned.find(op);
        if (pos != std::string::npos) {
            return parse_logical_formula(cleaned, op, pos);
        }
    }
    
    // Default to true if parsing fails
    return PresburgerFormula::equal(
        PresburgerTerm(1), 
        PresburgerTerm(1)
    );
}

std::unique_ptr<PresburgerFormula> GGGTemporalGameManager::parse_existential_formula(const std::string& formula_str) {
    // Extract variable name and inner formula
    std::regex exists_pattern(R"(exists\s+(\w+)\s*:\s*(.+))");
    std::smatch match;
    
    if (std::regex_match(formula_str, match, exists_pattern)) {
        std::string var_name = match[1].str();
        std::string inner_formula_str = match[2].str();
        auto inner_formula = parse_constraint(inner_formula_str);
        return PresburgerFormula::exists(var_name, std::move(inner_formula));
    }
    
    // Default fallback
    return PresburgerFormula::equal(PresburgerTerm(1), PresburgerTerm(1));
}

std::unique_ptr<PresburgerFormula> GGGTemporalGameManager::parse_comparison_formula(const std::string& formula_str, const std::string& op, size_t pos) {
    std::string left_str = formula_str.substr(0, pos);
    std::string right_str = formula_str.substr(pos + op.length());
    
    auto left_term = parse_presburger_term(left_str);
    auto right_term = parse_presburger_term(right_str);
    
    if (op == ">=") {
        return PresburgerFormula::greaterequal(*left_term, *right_term);
    } else if (op == "<=") {
        return PresburgerFormula::lessequal(*left_term, *right_term);
    } else if (op == ">") {
        return PresburgerFormula::greater(*left_term, *right_term);
    } else if (op == "<") {
        return PresburgerFormula::less(*left_term, *right_term);
    } else if (op == "==" || op == "=") {
        return PresburgerFormula::equal(*left_term, *right_term);
    } else if (op == "!=") {
        // Implement != as NOT(equal)
        auto equal_formula = PresburgerFormula::equal(*left_term, *right_term);
        return PresburgerFormula::not_formula(std::move(equal_formula));
    } else {
        return PresburgerFormula::equal(*left_term, *right_term);
    }
}

std::unique_ptr<PresburgerFormula> GGGTemporalGameManager::parse_logical_formula(const std::string& formula_str, const std::string& op, size_t pos) {
    std::string left_str = formula_str.substr(0, pos);
    std::string right_str = formula_str.substr(pos + op.length());
    
    auto left_formula = parse_constraint(left_str);
    auto right_formula = parse_constraint(right_str);
    
    if (op == "&&") {
        std::vector<std::unique_ptr<PresburgerFormula>> formulas;
        formulas.push_back(std::move(left_formula));
        formulas.push_back(std::move(right_formula));
        return PresburgerFormula::and_formula(std::move(formulas));
    } else if (op == "||") {
        std::vector<std::unique_ptr<PresburgerFormula>> formulas;
        formulas.push_back(std::move(left_formula));
        formulas.push_back(std::move(right_formula));
        return PresburgerFormula::or_formula(std::move(formulas));
    }
    
    return PresburgerFormula::equal(PresburgerTerm(1), PresburgerTerm(1));
}

std::unique_ptr<PresburgerFormula> GGGTemporalGameManager::parse_modulus_constraint(const std::string& formula_str, size_t mod_pos) {
    // Parse expressions like "expr mod m == r"
    std::string expr_str = formula_str.substr(0, mod_pos);
    std::string remainder_str = formula_str.substr(mod_pos + 3); // skip "mod"
    
    // Find the == operator
    auto eq_pos = remainder_str.find("==");
    if (eq_pos == std::string::npos) {
        return PresburgerFormula::equal(PresburgerTerm(1), PresburgerTerm(1));
    }
    
    std::string modulus_str = remainder_str.substr(0, eq_pos);
    std::string result_str = remainder_str.substr(eq_pos + 2);
    
    auto expr_term = parse_presburger_term(expr_str);
    int modulus = std::stoi(modulus_str);
    int result = std::stoi(result_str);
    
    return PresburgerFormula::modulus(*expr_term, modulus, result);
}

std::unique_ptr<PresburgerFormula> GGGTemporalGameManager::parse_percent_modulus_constraint(const std::string& formula_str, size_t percent_pos) {
    // Parse expressions like "expr%m==r"
    std::string expr_str = formula_str.substr(0, percent_pos);
    std::string remainder_str = formula_str.substr(percent_pos + 1); // skip "%"
    
    // Find the == operator
    auto eq_pos = remainder_str.find("==");
    if (eq_pos == std::string::npos) {
        return PresburgerFormula::equal(PresburgerTerm(1), PresburgerTerm(1));
    }
    
    std::string modulus_str = remainder_str.substr(0, eq_pos);
    std::string result_str = remainder_str.substr(eq_pos + 2);
    
    auto expr_term = parse_presburger_term(expr_str);
    int modulus = std::stoi(modulus_str);
    int result = std::stoi(result_str);
    
    return PresburgerFormula::modulus(*expr_term, modulus, result);
}

std::unique_ptr<PresburgerTerm> GGGTemporalGameManager::parse_presburger_term(const std::string& term_str) {
    // Handle simple constant
    if (std::all_of(term_str.begin(), term_str.end(), [](char c) { return std::isdigit(c) || c == '-'; })) {
        return std::make_unique<PresburgerTerm>(std::stoi(term_str));
    }
    
    // Handle simple variable
    if (std::all_of(term_str.begin(), term_str.end(), [](char c) { return std::isalnum(c) || c == '_'; })) {
        return std::make_unique<PresburgerTerm>(term_str);
    }
    
    // Handle coefficient * variable (e.g., "2*time")
    auto mult_pos = term_str.find('*');
    if (mult_pos != std::string::npos) {
        std::string coeff_str = term_str.substr(0, mult_pos);
        std::string var_str = term_str.substr(mult_pos + 1);
        
        if (std::all_of(coeff_str.begin(), coeff_str.end(), [](char c) { return std::isdigit(c) || c == '-'; }) &&
            std::all_of(var_str.begin(), var_str.end(), [](char c) { return std::isalnum(c) || c == '_'; })) {
            return std::make_unique<PresburgerTerm>(var_str, std::stoi(coeff_str));
        }
    }
    
    // Default to constant 0
    return std::make_unique<PresburgerTerm>(0);
}

} // namespace graphs
} // namespace ggg
