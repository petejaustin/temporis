#include "temporal_game_application.hpp"
#include "temporal_game_demo.hpp"
#include <iostream>

TemporalGameApplication::TemporalGameApplication() {
    initialize();
}

int TemporalGameApplication::run(int argc, char* argv[]) {
    if (argc > 1) {
        std::string filename = argv[1];
        return load_and_analyze_file(filename) ? 0 : 1;
    } else {
        run_demo_mode();
        return 0;
    }
}

bool TemporalGameApplication::load_and_analyze_file(const std::string& filename) {
    print_header(filename);
    
    // Parse the DOT file
    if (!parser_->parse_file(filename, *manager_)) {
        std::cerr << "Failed to parse file: " << filename << std::endl;
        return false;
    }
    
    // Generate full analysis report
    analyzer_->generate_full_report();
    
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
