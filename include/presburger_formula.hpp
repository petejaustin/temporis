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
    enum Type { EQUAL, GREATEREQUAL, LESSEQUAL, GREATER, LESS, AND, OR, NOT, EXISTS, MODULUS };

private:
    Type type_;
    PresburgerTerm left_;
    PresburgerTerm right_;
    std::vector<std::shared_ptr<PresburgerFormula>> children_;
    std::string existential_var_;
    int modulus_;
    int remainder_;

public:
    // Default constructor for serialization
    PresburgerFormula();
    
    PresburgerFormula(Type t, const PresburgerTerm& l, const PresburgerTerm& r);
    
    // Copy constructor
    PresburgerFormula(const PresburgerFormula& other);
    
    // Copy assignment operator  
    PresburgerFormula& operator=(const PresburgerFormula& other);
    
    // Move constructor
    PresburgerFormula(PresburgerFormula&& other) noexcept = default;
    
    // Move assignment operator
    PresburgerFormula& operator=(PresburgerFormula&& other) noexcept = default;
    
    static std::shared_ptr<PresburgerFormula> equal(const PresburgerTerm& left, const PresburgerTerm& right);
    static std::shared_ptr<PresburgerFormula> greaterequal(const PresburgerTerm& left, const PresburgerTerm& right);
    static std::shared_ptr<PresburgerFormula> lessequal(const PresburgerTerm& left, const PresburgerTerm& right);
    static std::shared_ptr<PresburgerFormula> greater(const PresburgerTerm& left, const PresburgerTerm& right);
    static std::shared_ptr<PresburgerFormula> less(const PresburgerTerm& left, const PresburgerTerm& right);
    static std::shared_ptr<PresburgerFormula> modulus(const PresburgerTerm& expr, int modulus, int remainder);
    static std::shared_ptr<PresburgerFormula> and_formula(std::vector<std::shared_ptr<PresburgerFormula>> formulas);
    static std::shared_ptr<PresburgerFormula> or_formula(std::vector<std::shared_ptr<PresburgerFormula>> formulas);
    static std::shared_ptr<PresburgerFormula> not_formula(std::shared_ptr<PresburgerFormula> formula);
    static std::shared_ptr<PresburgerFormula> exists(const std::string& var, std::shared_ptr<PresburgerFormula> formula);
    
    std::string to_string() const;
    bool evaluate(const std::map<std::string, int>& values) const;
    
private:
    int evaluate_term(const PresburgerTerm& term, const std::map<std::string, int>& values) const;
};

// Stream operators for Boost lexical_cast support
std::ostream& operator<<(std::ostream& os, const PresburgerFormula& formula);
std::istream& operator>>(std::istream& is, PresburgerFormula& formula);

} // namespace graphs
} // namespace ggg
