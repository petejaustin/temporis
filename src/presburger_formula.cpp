#include "presburger_formula.hpp"

namespace ggg {
namespace graphs {

PresburgerFormula::PresburgerFormula(Type t, const PresburgerTerm& l, const PresburgerTerm& r) 
    : type_(t), left_(l), right_(r), modulus_(0), remainder_(0) {}

std::unique_ptr<PresburgerFormula> PresburgerFormula::equal(const PresburgerTerm& left, const PresburgerTerm& right) {
    return std::make_unique<PresburgerFormula>(EQUAL, left, right);
}

std::unique_ptr<PresburgerFormula> PresburgerFormula::greaterequal(const PresburgerTerm& left, const PresburgerTerm& right) {
    return std::make_unique<PresburgerFormula>(GREATEREQUAL, left, right);
}

std::unique_ptr<PresburgerFormula> PresburgerFormula::lessequal(const PresburgerTerm& left, const PresburgerTerm& right) {
    return std::make_unique<PresburgerFormula>(LESSEQUAL, left, right);
}

std::unique_ptr<PresburgerFormula> PresburgerFormula::greater(const PresburgerTerm& left, const PresburgerTerm& right) {
    return std::make_unique<PresburgerFormula>(GREATER, left, right);
}

std::unique_ptr<PresburgerFormula> PresburgerFormula::less(const PresburgerTerm& left, const PresburgerTerm& right) {
    return std::make_unique<PresburgerFormula>(LESS, left, right);
}

std::unique_ptr<PresburgerFormula> PresburgerFormula::modulus(const PresburgerTerm& expr, int modulus, int remainder) {
    auto result = std::make_unique<PresburgerFormula>(MODULUS, expr, PresburgerTerm(0));
    result->modulus_ = modulus;
    result->remainder_ = remainder;
    return result;
}

std::unique_ptr<PresburgerFormula> PresburgerFormula::and_formula(std::vector<std::unique_ptr<PresburgerFormula>> formulas) {
    auto result = std::make_unique<PresburgerFormula>(AND, PresburgerTerm(0), PresburgerTerm(0));
    result->children_ = std::move(formulas);
    return result;
}

std::unique_ptr<PresburgerFormula> PresburgerFormula::or_formula(std::vector<std::unique_ptr<PresburgerFormula>> formulas) {
    auto result = std::make_unique<PresburgerFormula>(OR, PresburgerTerm(0), PresburgerTerm(0));
    result->children_ = std::move(formulas);
    return result;
}

std::unique_ptr<PresburgerFormula> PresburgerFormula::not_formula(std::unique_ptr<PresburgerFormula> formula) {
    auto result = std::make_unique<PresburgerFormula>(NOT, PresburgerTerm(0), PresburgerTerm(0));
    result->children_.push_back(std::move(formula));
    return result;
}

std::unique_ptr<PresburgerFormula> PresburgerFormula::exists(const std::string& var, std::unique_ptr<PresburgerFormula> formula) {
    auto result = std::make_unique<PresburgerFormula>(EXISTS, PresburgerTerm(0), PresburgerTerm(0));
    result->existential_var_ = var;
    result->children_.push_back(std::move(formula));
    return result;
}

std::string PresburgerFormula::to_string() const {
    switch (type_) {
        case EQUAL: return left_.to_string() + " = " + right_.to_string();
        case GREATEREQUAL: return left_.to_string() + " >= " + right_.to_string();
        case LESSEQUAL: return left_.to_string() + " <= " + right_.to_string();
        case GREATER: return left_.to_string() + " > " + right_.to_string();
        case LESS: return left_.to_string() + " < " + right_.to_string();
        case MODULUS: return left_.to_string() + " ≡ " + std::to_string(remainder_) + " (mod " + std::to_string(modulus_) + ")";
        case AND: return "AND(...)";
        case OR: return "OR(...)";
        case NOT: return "NOT(...)";
        case EXISTS: return "∃" + existential_var_ + ". (...)";
        default: return "unknown";
    }
}

bool PresburgerFormula::evaluate(const std::map<std::string, int>& values) const {
    switch (type_) {
        case EQUAL: {
            int left_val = evaluate_term(left_, values);
            int right_val = evaluate_term(right_, values);
            return left_val == right_val;
        }
        case GREATEREQUAL: {
            int left_val = evaluate_term(left_, values);
            int right_val = evaluate_term(right_, values);
            return left_val >= right_val;
        }
        case LESSEQUAL: {
            int left_val = evaluate_term(left_, values);
            int right_val = evaluate_term(right_, values);
            return left_val <= right_val;
        }
        case GREATER: {
            int left_val = evaluate_term(left_, values);
            int right_val = evaluate_term(right_, values);
            return left_val > right_val;
        }
        case LESS: {
            int left_val = evaluate_term(left_, values);
            int right_val = evaluate_term(right_, values);
            return left_val < right_val;
        }
        case MODULUS: {
            int expr_val = evaluate_term(left_, values);
            return (expr_val % modulus_) == remainder_;
        }
        case AND: {
            for (const auto& child : children_) {
                if (!child->evaluate(values)) {
                    return false;
                }
            }
            return true;
        }
        case OR: {
            for (const auto& child : children_) {
                if (child->evaluate(values)) {
                    return true;
                }
            }
            return false;
        }
        case NOT: {
            if (children_.empty()) {
                return false;
            }
            return !children_[0]->evaluate(values);
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

int PresburgerFormula::evaluate_term(const PresburgerTerm& term, const std::map<std::string, int>& values) const {
    int result = term.constant_;
    
    // Add contribution from each variable
    for (const auto& [var, coeff] : term.coefficients_) {
        auto it = values.find(var);
        if (it != values.end()) {
            result += coeff * it->second;
        }
        // Unknown variables default to 0 (no contribution)
    }
    
    return result;
}

} // namespace graphs
} // namespace ggg
