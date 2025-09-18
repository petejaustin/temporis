#pragma once

#include "presburger_term.hpp"
#include <memory>
#include <vector>
#include <map>
#include <string>

namespace ggg {
namespace graphs {

/**
 * @brief Presburger arithmetic formula representation
 */
class PresburgerFormula {
public:
    enum Type { EQUAL, GREATEREQUAL, LESSEQUAL, AND, EXISTS };

private:
    Type type_;
    PresburgerTerm left_;
    PresburgerTerm right_;
    std::vector<std::unique_ptr<PresburgerFormula>> children_;
    std::string existential_var_;

public:
    PresburgerFormula(Type t, const PresburgerTerm& l, const PresburgerTerm& r);
    
    static std::unique_ptr<PresburgerFormula> equal(const PresburgerTerm& left, const PresburgerTerm& right);
    static std::unique_ptr<PresburgerFormula> greaterequal(const PresburgerTerm& left, const PresburgerTerm& right);
    static std::unique_ptr<PresburgerFormula> lessequal(const PresburgerTerm& left, const PresburgerTerm& right);
    static std::unique_ptr<PresburgerFormula> and_formula(std::vector<std::unique_ptr<PresburgerFormula>> formulas);
    static std::unique_ptr<PresburgerFormula> exists(const std::string& var, std::unique_ptr<PresburgerFormula> formula);
    
    std::string to_string() const;
    bool evaluate(const std::map<std::string, int>& values) const;

private:
    int evaluate_term(const PresburgerTerm& term, const std::map<std::string, int>& values) const;
};

} // namespace graphs
} // namespace ggg
