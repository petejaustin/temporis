#pragma once

#include "temporal_game_manager.hpp"
#include "dot_parser.hpp"
#include "temporal_analyzer.hpp"
#include <string>
#include <memory>

using namespace ggg::graphs;

/**
 * @brief Main application controller for the Temporis system
 * 
 * Handles command-line argument processing, file loading, and orchestrates
 * the various components of the temporal game analysis system.
 */
class TemporalGameApplication {
public:
    /**
     * @brief Construct a new Temporal Game Application
     */
    TemporalGameApplication();

    /**
     * @brief Run the application with command-line arguments
     * 
     * @param argc Argument count
     * @param argv Argument values
     * @return int Exit code (0 for success, non-zero for error)
     */
    int run(int argc, char* argv[]);

    /**
     * @brief Load and analyze a DOT file
     * 
     * @param filename Path to the DOT file
     * @return true if successful, false otherwise
     */
    bool load_and_analyze_file(const std::string& filename);

    /**
     * @brief Run the demo mode (when no file is provided)
     */
    void run_demo_mode();

private:
    std::unique_ptr<PresburgerTemporalGameManager> manager_;
    std::unique_ptr<PresburgerTemporalDotParser> parser_;
    std::unique_ptr<TemporalAnalyzer> analyzer_;

    /**
     * @brief Initialize the application components
     */
    void initialize();

    /**
     * @brief Print application header
     * 
     * @param filename Optional filename being processed
     */
    void print_header(const std::string& filename = "") const;
};
