#include "temporal_game_application.hpp"
#include "temporal_game_demo.hpp"
#include <iostream>

TemporalGameApplication::TemporalGameApplication() {
    initialize();
}

int TemporalGameApplication::run(int argc, char* argv[]) {
    // Parse command line arguments
    bool verbose = false;
    bool validate_only = false;
    std::string filename;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--validate" || arg == "--check-format") {
            validate_only = true;
        } else if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        } else if (arg.find(".dot") != std::string::npos) {
            filename = arg;
        }
    }
    
    if (!filename.empty()) {
        // If validation mode, just validate and exit
        if (validate_only) {
            bool is_valid = parser_->validate_file_format_with_report(filename);
            return is_valid ? 0 : 1;
        }
        
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
    
    // First validate the file format
    if (!parser_->validate_file_format(filename)) {
        std::cerr << "File format validation failed. Cannot proceed with analysis." << std::endl;
        return false;
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

void TemporalGameApplication::print_usage() const {
    std::cout << "Temporis - Presburger Arithmetic Temporal Reachability Game Solver" << std::endl;
    std::cout << "=================================================================" << std::endl;
    std::cout << std::endl;
    std::cout << "USAGE:" << std::endl;
    std::cout << "  temporis [OPTIONS] <input_file.dot>" << std::endl;
    std::cout << std::endl;
    std::cout << "OPTIONS:" << std::endl;
    std::cout << "  -v, --verbose          Enable verbose output with detailed analysis" << std::endl;
    std::cout << "  --validate             Validate file format and exit (no game solving)" << std::endl;
    std::cout << "  --check-format         Alias for --validate" << std::endl;
    std::cout << "  -h, --help             Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "VALIDATION CHECKS:" << std::endl;
    std::cout << "  The following validation checks are performed automatically:" << std::endl;
    std::cout << "  • Each vertex must have at least one outgoing edge" << std::endl;
    std::cout << "  • At least one vertex must be marked as target (target=1)" << std::endl;
    std::cout << "  • Every constraint must include temporal reasoning (use 'time')" << std::endl;
    std::cout << std::endl;
    std::cout << "EXAMPLES:" << std::endl;
    std::cout << "  temporis game.dot                    # Solve reachability game" << std::endl;
    std::cout << "  temporis --verbose game.dot          # Detailed analysis output" << std::endl;
    std::cout << "  temporis --validate game.dot         # Check file format only" << std::endl;
    std::cout << std::endl;
}
