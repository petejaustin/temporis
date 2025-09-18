#pragma once

#include <string>
#include <map>

namespace ggg {
namespace graphs {

/**
 * @brief Presburger arithmetic term representation supporting multiple variables
 */
class PresburgerTerm {
public:
    std::map<std::string, int> coefficients_;  // variable -> coefficient mapping
    int constant_;

    PresburgerTerm();
    PresburgerTerm(const std::string& var);
    PresburgerTerm(const std::string& var, int coefficient);
    PresburgerTerm(int val);
    
    PresburgerTerm operator+(const PresburgerTerm& other) const;
    PresburgerTerm operator*(int scalar) const;
    std::string to_string() const;
};

} // namespace graphs
} // namespace ggg
