#pragma once

#include "temporal_game_manager.hpp"
#include "presburger_formula.hpp"
#include <string>
#include <memory>
#include <map>

namespace ggg {
namespace graphs {

/**
 * @brief Parser for DOT files with Presburger temporal constraints
 */
class PresburgerTemporalDotParser {
private:
    std::map<std::string, PresburgerTemporalVertex> vertex_map_;

public:
    bool parse_file(const std::string& filename, PresburgerTemporalGameManager& manager);

private:
    std::unique_ptr<PresburgerFormula> parse_constraint(const std::string& constraint_str);
    std::unique_ptr<PresburgerFormula> parse_existential_formula(const std::string& formula_str);
    std::unique_ptr<PresburgerFormula> parse_comparison_formula(const std::string& formula_str, const std::string& op, size_t pos);
    std::unique_ptr<PresburgerFormula> parse_logical_formula(const std::string& formula_str, const std::string& op, size_t pos);
    std::unique_ptr<PresburgerTerm> parse_presburger_term(const std::string& term_str);
    std::unique_ptr<PresburgerFormula> parse_arithmetic_constraint(const std::string& constraint_str);
    PresburgerTerm parse_linear_expression(const std::string& expr);
};

} // namespace graphs
} // namespace ggg
