#include "ggg_temporal_solver.hpp"
#include "ggg_temporal_graph.hpp"
#include "libggg/utils/solver_wrapper.hpp"
#include <iostream>
#include <iomanip>

namespace {
    bool g_verbose = false;
    
    void error(const std::string& msg) { std::cerr << "Error: " << msg << std::endl; }
    void info(const std::string& msg) { if (g_verbose) std::cout << msg << std::endl; }
}

namespace ggg {
namespace graphs {

} // namespace graphs
} // namespace ggg

/**
 * @brief Standalone solver executable using GGG infrastructure
 */
class TemporalReachabilityExecutor {
private:
    std::shared_ptr<ggg::graphs::GGGTemporalGameManager> manager_;
    std::shared_ptr<ggg::graphs::GGGReachabilityObjective> objective_;
    
public:
    TemporalReachabilityExecutor() 
        : manager_(std::make_shared<ggg::graphs::GGGTemporalGameManager>()) {}
    
    int run(int argc, char* argv[]) {
        // Parse command line arguments
        bool verbose = false;
        bool debug = false;
        bool validate_only = false;
        bool csv_output = false;
        bool time_only = false;
        std::string filename;
        int user_time_bound = -1;
        std::string solver_type = "reachability"; // Default solver
        
        // Set up logging based on verbosity
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--verbose" || arg == "-v") {
                verbose = true;
                g_verbose = true;
            } else if (arg == "--debug" || arg == "-d") {
                debug = true;
                verbose = true;
                g_verbose = true;
            } else if (arg == "--validate" || arg == "--check-format") {
                validate_only = true;
            } else if (arg == "--csv") {
                csv_output = true;
            } else if (arg == "--time-only") {
                time_only = true;
            } else if (arg == "--help" || arg == "-h") {
                print_usage();
                return 0;
            } else if (arg == "--solver" || arg == "-s") {
                if (i + 1 < argc) {
                    solver_type = argv[++i];
                    if (solver_type != "reachability" && solver_type != "expansion") {
                        error("Invalid solver: " + solver_type);
                        return 1;
                    }
                } else {
                    error("--solver requires a value");
                    return 1;
                }
            } else if (arg == "--time-bound" || arg == "-t") {
                if (i + 1 < argc) {
                    try {
                        user_time_bound = std::stoi(argv[++i]);
                        if (user_time_bound <= 0) {
                            error("Time bound must be positive");
                            return 1;
                        }
                    } catch (const std::exception&) {
                        error("Invalid time bound");
                        return 1;
                    }
                } else {
                    error("--time-bound requires a value");
                    return 1;
                }
            } else if (arg.find(".dot") != std::string::npos) {
                filename = arg;
            }
        }
        
        if (filename.empty()) {
            error("No input file specified");
            print_usage();
            return 1;
        }
        
        if (!manager_->load_from_dot_file(filename)) {
            error("Failed to load game");
            return 1;
        }
        
        if (validate_only) {
            bool valid = manager_->validate_game_structure();
            std::cout << (valid ? "Valid" : "Invalid") << std::endl;
            return valid ? 0 : 1;
        }
        
        auto targets = manager_->get_target_vertices();
        if (targets.empty()) {
            error("No target vertices found");
            return 1;
        }
        
        objective_ = std::make_shared<ggg::graphs::GGGReachabilityObjective>(
            ggg::graphs::GGGReachabilityObjective::Type::REACHABILITY, targets);
        
        // Create solver based on user selection
        std::shared_ptr<ggg::solvers::Solver<ggg::graphs::GGGTemporalGraph, ggg::solvers::RSSolution<ggg::graphs::GGGTemporalGraph>>> solver;
        
        int time_bound = user_time_bound > 0 ? user_time_bound : 50;
        
        if (solver_type == "expansion") {
            solver = std::make_shared<ggg::solvers::ExpansionSolver>(
                manager_, objective_, time_bound, verbose);
        } else { // Default to reachability solver
            solver = std::make_shared<ggg::solvers::GGGTemporalReachabilitySolver>(
                manager_, objective_, time_bound, verbose);
        }
        
        if (!csv_output && !time_only) {
            info("Solver: " + solver->get_name());
        }
        
        // Solve the game
        auto solution = solver->solve(*manager_->graph());
        
        // Handle different output modes - statistics depend on solver type
        ggg::solvers::SolverStatistics stats;
        if (solver_type == "expansion") {
            auto expansion_solver = std::dynamic_pointer_cast<ggg::solvers::ExpansionSolver>(solver);
            stats = expansion_solver->get_statistics();
        } else {
            auto reachability_solver = std::dynamic_pointer_cast<ggg::solvers::GGGTemporalReachabilitySolver>(solver);
            stats = reachability_solver->get_statistics();
        }
        
        if (csv_output) {
            output_csv(solution, stats, filename);
        } else if (time_only) {
            output_time_only(stats);
        } else {
            // Standard output mode
            if (verbose) {
                output_statistics(stats);
            }
            output_solution(solution, verbose);
        }
        
        return 0;
    }
    
private:
    void print_usage() const {
        std::cout << "Temporis - GGG-Compatible Presburger Temporal Reachability Solver\n";
        std::cout << "==================================================================\n\n";
        std::cout << "USAGE:\n";
        std::cout << "  temporis [OPTIONS] <input_file.dot>\n\n";
        std::cout << "OPTIONS:\n";
        std::cout << "  -v, --verbose          Enable verbose output\n";
        std::cout << "  -d, --debug            Enable debug output (includes verbose)\n";
        std::cout << "  -t, --time-bound N     Set solver time bound\n";
        std::cout << "  -s, --solver TYPE      Choose solver (reachability|expansion)\n";
        std::cout << "  --validate             Validate file format only\n";
        std::cout << "  --csv                  Output results in CSV format\n";
        std::cout << "  --time-only            Output only timing information\n";
        std::cout << "  -h, --help             Show this help\n\n";
        std::cout << "EXAMPLES:\n";
        std::cout << "  temporis game.dot                      # Solve with reachability solver\n";
        std::cout << "  temporis --solver expansion game.dot   # Solve with expansion solver\n";
        std::cout << "  temporis --verbose game.dot            # Detailed output\n";
        std::cout << "  temporis -t 100 game.dot               # Custom time bound\n";
    }
    
    void output_solution(const ggg::solvers::RSSolution<ggg::graphs::GGGTemporalGraph>& solution, bool verbose) {
        std::cout << "\n=== Solution ===\n";
        std::cout << "Status: " << (solution.is_solved() ? "Solved" : "Unsolved") << std::endl;
        std::cout << "Valid: " << (solution.is_valid() ? "Yes" : "No") << std::endl;
        
        // Always show winning regions (this is the main output we care about)
        if (solution.is_solved()) {
            std::cout << "\nWinning Regions:\n";
            
            auto [vertex_begin, vertex_end] = boost::vertices(*manager_->graph());
            for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
                auto vertex = *vertex_it;
                const auto& props = (*manager_->graph())[vertex];
                
                std::cout << "  " << props.name << ": ";
                if (solution.is_won_by_player0(vertex)) {
                    std::cout << "Player 0";
                    if (verbose && solution.has_strategy(vertex)) {
                        auto strategy_vertex = solution.get_strategy(vertex);
                        if (strategy_vertex != boost::graph_traits<ggg::graphs::GGGTemporalGraph>::null_vertex()) {
                            std::cout << " -> " << (*manager_->graph())[strategy_vertex].name;
                        }
                    }
                } else if (solution.is_won_by_player1(vertex)) {
                    std::cout << "Player 1";
                } else {
                    std::cout << "Undetermined";
                }
                std::cout << std::endl;
            }
        }
    }
    
    void output_statistics(const ggg::solvers::SolverStatistics& stats) {
        std::cout << "\n=== Solver Statistics ===\n";
        std::cout << "State space exploration:\n";
        std::cout << "  States explored: " << stats.states_explored << "\n";
        std::cout << "  States pruned: " << stats.states_pruned << "\n";
        std::cout << "  Max time reached: " << stats.max_time_reached << "\n";
        
        std::cout << "\nConstraint evaluation:\n";
        std::cout << "  Total evaluations: " << stats.constraint_evaluations << "\n";
        std::cout << "  Successful: " << stats.constraint_passes << "\n";
        std::cout << "  Failed: " << stats.constraint_failures << "\n";
        std::cout << "  Success ratio: " << std::fixed << std::setprecision(2) 
                  << (stats.constraint_success_ratio() * 100) << "%\n";
        
        std::cout << "\nMemoization performance:\n";
        std::cout << "  Cache hits: " << stats.cache_hits << "\n";
        std::cout << "  Cache misses: " << stats.cache_misses << "\n";
        std::cout << "  Hit ratio: " << std::fixed << std::setprecision(2) 
                  << (stats.cache_hit_ratio() * 100) << "%\n";
        
        std::cout << "\nTiming (seconds):\n";
        std::cout << "  Total solve time: " << std::fixed << std::setprecision(4) 
                  << stats.total_solve_time.count() << "s\n";
        std::cout << "  Constraint evaluation: " << std::fixed << std::setprecision(4) 
                  << stats.constraint_eval_time.count() << "s\n";
        std::cout << "  Graph traversal: " << std::fixed << std::setprecision(4) 
                  << stats.graph_traversal_time.count() << "s\n";
        std::cout << std::endl;
    }

    void output_csv(const ggg::solvers::RSSolution<ggg::graphs::GGGTemporalGraph>& solution, 
                    const ggg::solvers::SolverStatistics& stats, 
                    const std::string& filename) {
        // Extract filename without path and extension for solver identification
        std::string base_filename = filename;
        size_t last_slash = base_filename.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            base_filename = base_filename.substr(last_slash + 1);
        }
        size_t last_dot = base_filename.find_last_of(".");
        if (last_dot != std::string::npos) {
            base_filename = base_filename.substr(0, last_dot);
        }

        // Output CSV format compatible with GGG benchmark tools
        // Format: solver,game,status,solve_time,constraint_eval_time,graph_traversal_time,vertices_explored
        std::cout << "Backwards Temporal Attractor Solver,"
                  << base_filename << ","
                  << (solution.is_valid() ? "solved" : "unsolved") << ","
                  << std::fixed << std::setprecision(6) << stats.total_solve_time.count() << ","
                  << std::fixed << std::setprecision(6) << stats.constraint_eval_time.count() << ","
                  << std::fixed << std::setprecision(6) << stats.graph_traversal_time.count() << ","
                  << stats.states_explored << std::endl;
    }

    void output_time_only(const ggg::solvers::SolverStatistics& stats) {
        // Output only the total solve time (used by benchmark tools)
        std::cout << std::fixed << std::setprecision(6) << stats.total_solve_time.count() << std::endl;
    }
};

int main(int argc, char* argv[]) {
    TemporalReachabilityExecutor executor;
    return executor.run(argc, argv);
}
