#include "temporal_game_application.hpp"
#include "temporal_game_demo.hpp"
#include <iostream>

TemporalGameApplication::TemporalGameApplication() {
    initialize();
}

int TemporalGameApplication::run(int argc, char* argv[]) {
    if (argc > 1) {
        std::string filename = argv[1];
        
        // Try to parse with objective first
        std::shared_ptr<ReachabilityObjective> objective;
        bool has_objective = parser_->parse_file_with_objective(filename, *manager_, objective);
        
        if (has_objective && objective) {
            // File parsed successfully with objective, now just do the analysis and solving
            print_header(filename);
            analyzer_->generate_full_report();
            
            std::cout << "\n=== Reachability Game Solving ===\n";
            
            // Create solver
            TemporalReachabilitySolver solver(*manager_, objective, 30);
            
            // Compute winning regions for all vertices
            auto [player0_winning, player1_winning] = solver.compute_winning_regions(0);
            solver.print_winning_regions_analysis(player0_winning, player1_winning);
            
            // Also solve from a specific initial vertex if one is identifiable
            PresburgerTemporalVertex initial_vertex;
            bool found_initial = false;
            
            auto [vertices_begin, vertices_end] = boost::vertices(manager_->graph());
            for (auto vertex_it = vertices_begin; vertex_it != vertices_end; ++vertex_it) {
                if (manager_->graph()[*vertex_it].name == "v0") {
                    initial_vertex = *vertex_it;
                    found_initial = true;
                    break;
                }
            }
            
            if (found_initial) {
                std::cout << "\n=== Detailed Analysis from Initial Vertex v0 ===\n";
                auto solution = solver.solve(initial_vertex, 0);
                GameState initial_state{initial_vertex, 0};
                solver.print_solution_analysis(solution, initial_state);
            }
            
            return true;
        } else {
            // No objective found, parse as regular file
            return load_and_analyze_file(filename) ? 0 : 1;
        }
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

bool TemporalGameApplication::load_and_solve_reachability_game(const std::string& filename) {
    print_header(filename);
    
    std::shared_ptr<ReachabilityObjective> objective;
    
    // Parse the DOT file with objective
    if (!parser_->parse_file_with_objective(filename, *manager_, objective)) {
        std::cerr << "Failed to parse file: " << filename << std::endl;
        return false;
    }
    
    // Generate basic analysis report
    analyzer_->generate_full_report();
    
    // Solve reachability game if objective was found
    if (objective) {
        std::cout << "\n=== Reachability Game Solving ===\n";
        
        // Create solver
        TemporalReachabilitySolver solver(*manager_, objective, 30); // 30 time steps max
        
        // Compute winning regions for all vertices
        auto [player0_winning, player1_winning] = solver.compute_winning_regions(0);
        solver.print_winning_regions_analysis(player0_winning, player1_winning);
        
        // Also solve from a specific initial vertex if one is identifiable
        PresburgerTemporalVertex initial_vertex;
        bool found_initial = false;
        
        auto [vertices_begin, vertices_end] = boost::vertices(manager_->graph());
        for (auto vertex_it = vertices_begin; vertex_it != vertices_end; ++vertex_it) {
            if (manager_->graph()[*vertex_it].name == "v0") {
                initial_vertex = *vertex_it;
                found_initial = true;
                break;
            }
        }
        
        if (found_initial) {
            std::cout << "\n=== Detailed Analysis from Initial Vertex v0 ===\n";
            auto solution = solver.solve(initial_vertex, 0);
            GameState initial_state{initial_vertex, 0};
            solver.print_solution_analysis(solution, initial_state);
        }
    } else {
        std::cout << "\nNo reachability objective found in file. Use comment format: // OBJECTIVE: type targets [time_bound]\n";
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
