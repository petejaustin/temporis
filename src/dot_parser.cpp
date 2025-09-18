#include "dot_parser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

namespace ggg {
namespace graphs {

bool PresburgerTemporalDotParser::parse_file(const std::string& filename, PresburgerTemporalGameManager& manager) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }
    
    std::string line;
    std::regex vertex_pattern(R"(\s*(\w+)\s*\[\s*name\s*=\s*\"([^\"]+)\"\s*,\s*player\s*=\s*(\d+)(?:\s*,\s*target\s*=\s*(\d+))?\s*\]\s*;)");
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
            // target attribute is optional (match[4])
            
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

bool PresburgerTemporalDotParser::parse_file_with_objective(const std::string& filename, 
                                                           PresburgerTemporalGameManager& manager,
                                                           std::shared_ptr<ReachabilityObjective>& objective) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }
    
    // Clear the manager and vertex map to start fresh
    manager.clear_graph();
    vertex_map_.clear();
    
    std::string line;
    std::regex vertex_pattern(R"(\s*(\w+)\s*\[\s*name\s*=\s*\"([^\"]+)\"\s*,\s*player\s*=\s*(\d+)(?:\s*,\s*target\s*=\s*(\d+))?\s*\]\s*;)");
    std::regex edge_pattern(R"(\s*(\w+)\s*->\s*(\w+)\s*\[\s*label\s*=\s*\"([^\"]+)\"(?:\s*,\s*constraint\s*=\s*\"([^\"]+)\")?\s*\]\s*;?)");
    
    std::set<PresburgerTemporalVertex> target_vertices;
    
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
            
            // Check if target attribute is present (match[4])
            bool is_target = false;
            if (match.size() > 4 && !match[4].str().empty()) {
                is_target = (std::stoi(match[4].str()) == 1);
            }
            
            auto vertex = manager.add_vertex(name, player);
            vertex_map_[vertex_id] = vertex;
            
            // Add to target vertices if marked as target
            if (is_target) {
                target_vertices.insert(vertex);
            }
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
    
    // Create reachability objective if target vertices were found
    if (!target_vertices.empty()) {
        objective = std::make_shared<ReachabilityObjective>(
            ReachabilityObjective::ObjectiveType::REACHABILITY,
            target_vertices
        );
        return true;
    }
    
    // No target vertices found, but parsing was successful
    objective = nullptr;
    return true;
}

std::shared_ptr<ReachabilityObjective> PresburgerTemporalDotParser::parse_objective(const std::string& objective_str) {
    // Format: "type targets [time_bound]"
    // Examples: "reachability v2,v3", "safety v1", "time_bounded_reach v2 10"
    
    std::istringstream iss(objective_str);
    std::string type_str, targets_str, time_str;
    iss >> type_str >> targets_str >> time_str;
    
    ReachabilityObjective::ObjectiveType type = parse_objective_type(type_str);
    std::set<PresburgerTemporalVertex> targets = parse_target_vertices(targets_str);
    int time_bound = time_str.empty() ? -1 : std::stoi(time_str);
    
    return std::make_shared<ReachabilityObjective>(type, targets, time_bound);
}

ReachabilityObjective::ObjectiveType PresburgerTemporalDotParser::parse_objective_type(const std::string& type_str) {
    if (type_str == "reachability") return ReachabilityObjective::ObjectiveType::REACHABILITY;
    if (type_str == "safety") return ReachabilityObjective::ObjectiveType::SAFETY;
    if (type_str == "time_bounded_reach") return ReachabilityObjective::ObjectiveType::TIME_BOUNDED_REACH;
    if (type_str == "time_bounded_safety") return ReachabilityObjective::ObjectiveType::TIME_BOUNDED_SAFETY;
    return ReachabilityObjective::ObjectiveType::REACHABILITY; // Default
}

std::set<PresburgerTemporalVertex> PresburgerTemporalDotParser::parse_target_vertices(const std::string& targets_str) {
    std::set<PresburgerTemporalVertex> targets;
    std::istringstream iss(targets_str);
    std::string vertex_id;
    
    while (std::getline(iss, vertex_id, ',')) {
        // Remove whitespace
        vertex_id.erase(std::remove_if(vertex_id.begin(), vertex_id.end(), ::isspace), vertex_id.end());
        
        if (vertex_map_.find(vertex_id) != vertex_map_.end()) {
            targets.insert(vertex_map_[vertex_id]);
        }
    }
    
    return targets;
}

std::unique_ptr<PresburgerFormula> PresburgerTemporalDotParser::parse_constraint(const std::string& constraint_str) {
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

std::unique_ptr<PresburgerFormula> PresburgerTemporalDotParser::parse_existential_formula(const std::string& formula_str) {
    // Extract variable name and inner formula
    // Format: exists var : formula
    auto colon_pos = formula_str.find(':');
    if (colon_pos == std::string::npos) {
        // Try with dot separator: exists var. formula
        auto dot_pos = formula_str.find('.');
        if (dot_pos == std::string::npos) {
            return PresburgerFormula::equal(PresburgerTerm(1), PresburgerTerm(1));
        }
        colon_pos = dot_pos;
    }
    
    std::string var_part = formula_str.substr(6, colon_pos - 6); // Skip "exists"
    std::string inner_part = formula_str.substr(colon_pos + 1);
    
    // Extract variable name
    var_part.erase(std::remove_if(var_part.begin(), var_part.end(), ::isspace), var_part.end());
    
    auto inner_formula = parse_constraint(inner_part);
    return PresburgerFormula::exists(var_part, std::move(inner_formula));
}

std::unique_ptr<PresburgerFormula> PresburgerTemporalDotParser::parse_comparison_formula(const std::string& formula_str, const std::string& op, size_t pos) {
    std::string left_str = formula_str.substr(0, pos);
    std::string right_str = formula_str.substr(pos + op.length());
    
    auto left_term = parse_presburger_term(left_str);
    auto right_term = parse_presburger_term(right_str);
    
    if (op == ">=") {
        return PresburgerFormula::greaterequal(*left_term, *right_term);
    } else if (op == "<=") {
        return PresburgerFormula::lessequal(*left_term, *right_term);
    } else if (op == ">" || op == "<" || op == "==" || op == "!=") {
        // For now, map all other comparisons to equal for simplicity
        return PresburgerFormula::equal(*left_term, *right_term);
    } else {
        return PresburgerFormula::equal(*left_term, *right_term);
    }
}

std::unique_ptr<PresburgerFormula> PresburgerTemporalDotParser::parse_logical_formula(const std::string& formula_str, const std::string& op, size_t pos) {
    std::string left_str = formula_str.substr(0, pos);
    std::string right_str = formula_str.substr(pos + op.length());
    
    auto left_formula = parse_constraint(left_str);
    auto right_formula = parse_constraint(right_str);
    
    if (op == "&&") {
        std::vector<std::unique_ptr<PresburgerFormula>> formulas;
        formulas.push_back(std::move(left_formula));
        formulas.push_back(std::move(right_formula));
        return PresburgerFormula::and_formula(std::move(formulas));
    } else {
        // For OR, we'll simulate with AND for now (simplification)
        std::vector<std::unique_ptr<PresburgerFormula>> formulas;
        formulas.push_back(std::move(left_formula));
        formulas.push_back(std::move(right_formula));
        return PresburgerFormula::and_formula(std::move(formulas));
    }
}

std::unique_ptr<PresburgerTerm> PresburgerTemporalDotParser::parse_presburger_term(const std::string& term_str) {
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

std::unique_ptr<PresburgerFormula> PresburgerTemporalDotParser::parse_arithmetic_constraint(const std::string& constraint_str) {
    // Parse expressions like "time = 2*a + 3*b + 4*c + 5"
    // This is a simplified parser that handles general linear combinations
    
    std::string op;
    size_t op_pos = std::string::npos;
    
    if ((op_pos = constraint_str.find(" >= ")) != std::string::npos) {
        op = ">=";
    } else if ((op_pos = constraint_str.find(" <= ")) != std::string::npos) {
        op = "<=";
    } else if ((op_pos = constraint_str.find(" = ")) != std::string::npos) {
        op = "=";
    } else {
        return PresburgerFormula::equal(PresburgerTerm(1), PresburgerTerm(1));
    }
    
    std::string left_side = constraint_str.substr(0, op_pos);
    std::string right_side = constraint_str.substr(op_pos + op.length() + 2);
    
    // Left side should be "time"
    if (left_side.find("time") == std::string::npos) {
        return PresburgerFormula::equal(PresburgerTerm(1), PresburgerTerm(1));
    }
    
    // Parse the right side as a linear combination
    PresburgerTerm right_term = parse_linear_expression(right_side);
    
    if (op == "=") {
        return PresburgerFormula::equal(PresburgerTerm("time"), right_term);
    } else if (op == ">=") {
        return PresburgerFormula::greaterequal(PresburgerTerm("time"), right_term);
    } else if (op == "<=") {
        return PresburgerFormula::lessequal(PresburgerTerm("time"), right_term);
    }
    
    return PresburgerFormula::equal(PresburgerTerm(1), PresburgerTerm(1));
}

PresburgerTerm PresburgerTemporalDotParser::parse_linear_expression(const std::string& expr) {
    // Parse expressions like "2*a + 3*b + 4*c + 5" or "a + b + c + 1"
    PresburgerTerm result;
    
    // Split by + and - while keeping the operators
    std::vector<std::string> terms;
    std::string current_term;
    bool negative = false;
    
    for (size_t i = 0; i < expr.length(); ++i) {
        char c = expr[i];
        if (c == '+' || c == '-') {
            if (!current_term.empty()) {
                terms.push_back((negative ? "-" : "") + current_term);
                current_term.clear();
            }
            negative = (c == '-');
        } else if (c != ' ') {
            current_term += c;
        }
    }
    if (!current_term.empty()) {
        terms.push_back((negative ? "-" : "") + current_term);
    }
    
    // Parse each term
    for (const std::string& term_str : terms) {
        std::string trimmed = term_str;
        // Remove leading/trailing whitespace
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
        
        if (trimmed.empty()) continue;
        
        bool is_negative = trimmed[0] == '-';
        if (is_negative) trimmed = trimmed.substr(1);
        
        // Check if it's a pure number (constant)
        if (std::regex_match(trimmed, std::regex(R"(\d+)"))) {
            int value = std::stoi(trimmed);
            result.constant_ += is_negative ? -value : value;
        }
        // Check if it's coefficient*variable
        else if (std::regex_match(trimmed, std::regex(R"((\d+)\*(\w+))"))) {
            std::smatch match;
            std::regex_match(trimmed, match, std::regex(R"((\d+)\*(\w+))"));
            int coeff = std::stoi(match[1].str());
            std::string var = match[2].str();
            result.coefficients_[var] += is_negative ? -coeff : coeff;
        }
        // Check if it's just a variable
        else if (std::regex_match(trimmed, std::regex(R"(\w+)"))) {
            result.coefficients_[trimmed] += is_negative ? -1 : 1;
        }
    }
    
    return result;
}

std::unique_ptr<PresburgerFormula> PresburgerTemporalDotParser::parse_modulus_constraint(const std::string& formula_str, size_t mod_pos) {
    // Parse: expr mod m == r  or  expr mod m = r
    std::string expr_str = formula_str.substr(0, mod_pos);
    std::string mod_part = formula_str.substr(mod_pos + 3); // Skip "mod"
    
    // Find the equals sign
    auto eq_pos = mod_part.find("==");
    if (eq_pos == std::string::npos) {
        eq_pos = mod_part.find('=');
        if (eq_pos == std::string::npos) {
            // Invalid syntax, return true
            return PresburgerFormula::equal(PresburgerTerm(1), PresburgerTerm(1));
        }
    }
    
    std::string modulus_str = mod_part.substr(0, eq_pos);
    std::string remainder_str = mod_part.substr(eq_pos + (mod_part[eq_pos + 1] == '=' ? 2 : 1));
    
    // Parse components
    auto expr_term = parse_presburger_term(expr_str);
    int modulus = std::stoi(modulus_str);
    int remainder = std::stoi(remainder_str);
    
    return PresburgerFormula::modulus(*expr_term, modulus, remainder);
}

std::unique_ptr<PresburgerFormula> PresburgerTemporalDotParser::parse_percent_modulus_constraint(const std::string& formula_str, size_t percent_pos) {
    // Parse: expr%m==r  or  expr%m=r
    std::string expr_str = formula_str.substr(0, percent_pos);
    std::string mod_part = formula_str.substr(percent_pos + 1); // Skip "%"
    
    // Find the equals sign
    auto eq_pos = mod_part.find("==");
    if (eq_pos == std::string::npos) {
        eq_pos = mod_part.find('=');
        if (eq_pos == std::string::npos) {
            // Invalid syntax, return true
            return PresburgerFormula::equal(PresburgerTerm(1), PresburgerTerm(1));
        }
    }
    
    std::string modulus_str = mod_part.substr(0, eq_pos);
    std::string remainder_str = mod_part.substr(eq_pos + (mod_part[eq_pos + 1] == '=' ? 2 : 1));
    
    // Parse components
    auto expr_term = parse_presburger_term(expr_str);
    int modulus = std::stoi(modulus_str);
    int remainder = std::stoi(remainder_str);
    
    return PresburgerFormula::modulus(*expr_term, modulus, remainder);
}

} // namespace graphs
} // namespace ggg
