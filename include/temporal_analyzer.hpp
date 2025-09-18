#pragma once

#include "temporal_game_manager.hpp"
#include <iostream>
#include <string>

using namespace ggg::graphs;

/**
 * @brief Handles temporal analysis and reporting for Presburger temporal games
 * 
 * This class provides functionality to analyze temporal games over time ranges,
 * generate reports about game structure, and evaluate edge constraints.
 */
class TemporalAnalyzer {
public:
    /**
     * @brief Construct a new Temporal Analyzer
     * 
     * @param manager Reference to the temporal game manager
     */
    explicit TemporalAnalyzer(PresburgerTemporalGameManager& manager);

    /**
     * @brief Print comprehensive game structure information
     * 
     * Displays player vertices, edges, and basic game statistics
     */
    void print_game_structure() const;

    /**
     * @brief Analyze edge availability over a time range
     * 
     * @param start_time Starting time for analysis
     * @param end_time Ending time for analysis (inclusive)
     */
    void analyze_temporal_edges(int start_time, int end_time) const;

    /**
     * @brief Run a complete temporal analysis report
     * 
     * Combines game structure printing with temporal edge analysis
     * 
     * @param start_time Starting time for temporal analysis
     * @param end_time Ending time for temporal analysis
     */
    void generate_full_report(int start_time = 0, int end_time = 25) const;

    /**
     * @brief Print basic game statistics
     */
    void print_game_statistics() const;

private:
    PresburgerTemporalGameManager& manager_;
};
