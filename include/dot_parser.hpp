#pragma once

#include "temporal_game_manager.hpp"
#include "presburger_formula.hpp"
#include "reachability_objective.hpp"
#include <string>
#include <memory>
#include <map>

namespace ggg {
namespace graphs {

/**
 * @brief Parser for DOT files with Presburger temporal constraints and reachability objectives
 */
class PresburgerTemporalDotParser {
private:
    std::map<std::string, PresburgerTemporalVertex> vertex_map_;

public:
    bool parse_file(const std::string& filename, PresburgerTemporalGameManager& manager);
    bool parse_file_with_objective(const std::string& filename, 
                                  PresburgerTemporalGameManager& manager,
                                  std::shared_ptr<ReachabilityObjective>& objective);

private:
    std::unique_ptr<PresburgerFormula> parse_constraint(const std::string& constraint_str);
    std::unique_ptr<PresburgerFormula> parse_existential_formula(const std::string& formula_str);
    std::unique_ptr<PresburgerFormula> parse_modulus_constraint(const std::string& formula_str, size_t mod_pos);
    std::unique_ptr<PresburgerFormula> parse_percent_modulus_constraint(const std::string& formula_str, size_t percent_pos);
    std::unique_ptr<PresburgerFormula> parse_comparison_formula(const std::string& formula_str, const std::string& op, size_t pos);
    std::unique_ptr<PresburgerFormula> parse_logical_formula(const std::string& formula_str, const std::string& op, size_t pos);
    std::unique_ptr<PresburgerTerm> parse_presburger_term(const std::string& term_str);
    std::unique_ptr<PresburgerFormula> parse_arithmetic_constraint(const std::string& constraint_str);
    PresburgerTerm parse_linear_expression(const std::string& expr);
    
    // New methods for objective parsing
    std::shared_ptr<ReachabilityObjective> parse_objective(const std::string& objective_str);
    ReachabilityObjective::ObjectiveType parse_objective_type(const std::string& type_str);
    std::set<PresburgerTemporalVertex> parse_target_vertices(const std::string& targets_str);
};

} // namespace graphs
} // namespace ggg
