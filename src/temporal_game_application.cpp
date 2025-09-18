#include "temporal_game_application.hpp"
#include "temporal_game_demo.hpp"
#include <iostream>

TemporalGameApplication::TemporalGameApplication() {
    initialize();
}

int TemporalGameApplication::run(int argc, char* argv[]) {
    // Parse command line arguments
    bool verbose = false;
    std::string filename;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg.find(".dot") != std::string::npos) {
            filename = arg;
        }
    }
    
    if (!filename.empty()) {
        // Try to parse with objective first
        std::shared_ptr<ReachabilityObjective> objective;
        bool has_objective = parser_->parse_file_with_objective(filename, *manager_, objective);
        
        if (has_objective && objective) {
            // File parsed successfully with objective, now do the analysis and solving
            if (verbose) {
                print_header(filename);
                analyzer_->print_game_structure();
                analyzer_->analyze_temporal_edges(0, 25);  // Analyze first 26 time steps
            }
            
            std::cout << "\n=== Reachability Game Solving ===\n";
            
            // Create solver
            TemporalReachabilitySolver solver(*manager_, objective, 30);
            
            // Compute winning regions for all vertices
            auto [player0_winning, player1_winning] = solver.compute_winning_regions(0);
            solver.print_winning_regions_analysis(player0_winning, player1_winning);
            
            return true;
        } else {
            // No objective found, parse as regular file
            return load_and_analyze_file(filename, verbose) ? 0 : 1;
        }
    } else {
        run_demo_mode();
        return 0;
    }
}

bool TemporalGameApplication::load_and_analyze_file(const std::string& filename, bool verbose) {
    if (verbose) {
        print_header(filename);
    }
    
    // Parse the DOT file
    if (!parser_->parse_file(filename, *manager_)) {
        std::cerr << "Failed to parse file: " << filename << std::endl;
        return false;
    }
    
    // Generate analysis report based on verbosity level
    if (verbose) {
        analyzer_->generate_full_report();
    } else {
        // Basic output - just game stats
        analyzer_->print_game_statistics();
    }
    
    return true;
}

void TemporalGameApplication::run_demo_mode() {
    TemporalGameDemo::run_complete_demo();
}

void TemporalGameApplication::initialize() {
    manager_ = std::make_unique<PresburgerTemporalGameManager>();
    parser_ = std::make_unique<PresburgerTemporalDotParser>();
    analyzer_ = std::make_unique<TemporalAnalyzer>(*manager_);
}

void TemporalGameApplication::print_header(const std::string& filename) const {
    if (!filename.empty()) {
        std::cout << "Loading Presburger Arithmetic Temporal Game from: " << filename << "\n";
        std::cout << "==================================================\n\n";
    }
}
