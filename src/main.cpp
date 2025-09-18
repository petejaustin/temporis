#include "temporal_game_application.hpp"

/**
 * @brief Main entry point for the Temporis temporal game analysis system
 * 
 * This application can operate in two modes:
 * 1. File analysis mode: When provided with a DOT file argument
 * 2. Demo mode: When no arguments are provided
 * 
 * @param argc Argument count
 * @param argv Argument vector (argv[1] should be DOT file path if provided)
 * @return int Exit code (0 for success, non-zero for error)
 */
int main(int argc, char* argv[]) {
    TemporalGameApplication app;
    return app.run(argc, argv);
}
