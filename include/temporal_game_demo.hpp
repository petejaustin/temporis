#pragma once

#include "temporal_game_manager.hpp"
#include "presburger_term.hpp"
#include "presburger_formula.hpp"
#include <iostream>

using namespace ggg::graphs;

/**
 * @brief Provides demonstration and testing functionality for the temporal game system
 * 
 * This class contains various demo scenarios and tests to showcase the capabilities
 * of the modular temporal game system.
 */
class TemporalGameDemo {
public:
    /**
     * @brief Run the complete demo showcasing all modular components
     */
    static void run_complete_demo();

    /**
     * @brief Test PresburgerTerm functionality
     */
    static void test_presburger_terms();

    /**
     * @brief Test PresburgerFormula functionality
     */
    static void test_presburger_formulas();

    /**
     * @brief Create a sample game with vertices and edges
     * 
     * @return PresburgerTemporalGameManager with sample game loaded
     */
    static PresburgerTemporalGameManager create_sample_game();

    /**
     * @brief Test the modular game structure
     * 
     * @param manager Game manager to test
     */
    static void test_game_structure(const PresburgerTemporalGameManager& manager);
};
