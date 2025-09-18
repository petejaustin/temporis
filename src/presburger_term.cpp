#include "presburger_term.hpp"
#include <algorithm>
#include <cmath>

namespace ggg {
namespace graphs {

PresburgerTerm::PresburgerTerm(const std::string& var) : constant_(0) {
    coefficients_[var] = 1;
}

PresburgerTerm::PresburgerTerm(const std::string& var, int coefficient) : constant_(0) {
    coefficients_[var] = coefficient;
}

PresburgerTerm::PresburgerTerm(int val) : constant_(val) {}

PresburgerTerm::PresburgerTerm() : constant_(0) {}

PresburgerTerm PresburgerTerm::operator+(const PresburgerTerm& other) const {
    PresburgerTerm result = *this;
    result.constant_ += other.constant_;
    
    // Add coefficients for each variable
    for (const auto& [var, coeff] : other.coefficients_) {
        result.coefficients_[var] += coeff;
    }
    
    return result;
}

PresburgerTerm PresburgerTerm::operator*(int scalar) const {
    PresburgerTerm result = *this;
    result.constant_ *= scalar;
    
    for (auto& [var, coeff] : result.coefficients_) {
        coeff *= scalar;
    }
    
    return result;
}

std::string PresburgerTerm::to_string() const {
    std::string result;
    bool first = true;
    
    // Add variable terms
    for (const auto& [var, coeff] : coefficients_) {
        if (coeff == 0) continue;
        
        if (!first && coeff > 0) result += " + ";
        if (coeff < 0) result += " - ";
        
        int abs_coeff = std::abs(coeff);
        if (abs_coeff == 1) {
            result += var;
        } else {
            result += std::to_string(abs_coeff) + "*" + var;
        }
        first = false;
    }
    
    // Add constant term
    if (constant_ != 0 || first) {
        if (!first && constant_ > 0) result += " + ";
        if (constant_ < 0 && !first) result += " - ";
        
        if (first || constant_ < 0) {
            result += std::to_string(constant_);
        } else {
            result += std::to_string(constant_);
        }
    }
    
    return result.empty() ? "0" : result;
}

} // namespace graphs
} // namespace ggg
