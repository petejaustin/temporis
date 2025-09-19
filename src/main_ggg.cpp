#include "ggg_temporal_solver.hpp"
#include "ggg_temporal_graph.hpp"
#include "libggg/utils/solver_wrapper.hpp"
#include <iostream>

namespace ggg {
namespace graphs {

/**
 * @brief Parser function compatible with GGG solver wrapper
 */
std::shared_ptr<GGGTemporalGraph> parse_presburger_temporal_graph(const std::string& filename) {
    auto manager = std::make_shared<GGGTemporalGameManager>();
    
    if (!manager->load_from_dot_file(filename)) {
        return nullptr;
    }
    
    return manager->graph();
}

std::shared_ptr<GGGTemporalGraph> parse_presburger_temporal_graph(std::istream& in) {
    // For now, stream parsing not implemented - could extend DOT parser
    std::cerr << "Stream parsing not yet implemented for GGG temporal graphs\n";
    return nullptr;
}

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
        bool validate_only = false;
        std::string filename;
        int user_time_bound = -1;
        
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--verbose" || arg == "-v") {
                verbose = true;
            } else if (arg == "--validate" || arg == "--check-format") {
                validate_only = true;
            } else if (arg == "--help" || arg == "-h") {
                print_usage();
                return 0;
            } else if (arg == "--time-bound" || arg == "-t") {
                if (i + 1 < argc) {
                    try {
                        user_time_bound = std::stoi(argv[++i]);
                        if (user_time_bound <= 0) {
                            std::cerr << "Error: Time bound must be positive\n";
                            return 1;
                        }
                    } catch (const std::exception&) {
                        std::cerr << "Error: Invalid time bound value\n";
                        return 1;
                    }
                } else {
                    std::cerr << "Error: --time-bound requires a value\n";
                    return 1;
                }
            } else if (arg.find(".dot") != std::string::npos) {
                filename = arg;
            }
        }
        
        if (filename.empty()) {
            std::cerr << "Error: No input file specified\n";
            print_usage();
            return 1;
        }
        
        // Load the game
        if (!manager_->load_from_dot_file(filename)) {
            std::cerr << "Error: Failed to load game from " << filename << std::endl;
            return 1;
        }
        
        if (validate_only) {
            bool valid = manager_->validate_game_structure();
            std::cout << (valid ? "Valid" : "Invalid") << " game structure\n";
            return valid ? 0 : 1;
        }
        
        // Create objective from target vertices
        auto targets = manager_->get_target_vertices();
        if (targets.empty()) {
            std::cerr << "Error: No target vertices found\n";
            return 1;
        }
        
        objective_ = std::make_shared<ggg::graphs::GGGReachabilityObjective>(
            ggg::graphs::GGGReachabilityObjective::Type::REACHABILITY, targets);
        
        // Configure time bounds
        ggg::temporal::TimeBoundCalculator::Config time_config;
        time_config.verbose = verbose;
        time_config.user_override = user_time_bound;
        
        // Create and run solver
        auto solver = std::make_shared<ggg::solvers::GGGTemporalReachabilitySolver>(
            manager_, objective_, time_config);
        
        if (verbose) {
            std::cout << "Solver: " << solver->get_name() << std::endl;
            std::cout << "Graph: " << boost::num_vertices(*manager_->graph()) << " vertices, "
                      << boost::num_edges(*manager_->graph()) << " edges" << std::endl;
        }
        
        // Solve the game
        auto solution = solver->solve(*manager_->graph());
        
        // Output results
        output_solution(solution, verbose);
        
        return 0;
    }
    
private:
    void print_usage() const {
        std::cout << "Temporis - GGG-Compatible Presburger Temporal Reachability Solver\n";
        std::cout << "==================================================================\n\n";
        std::cout << "USAGE:\n";
        std::cout << "  temporis-ggg [OPTIONS] <input_file.dot>\n\n";
        std::cout << "OPTIONS:\n";
        std::cout << "  -v, --verbose          Enable verbose output\n";
        std::cout << "  -t, --time-bound N     Set solver time bound\n";
        std::cout << "  --validate             Validate file format only\n";
        std::cout << "  -h, --help             Show this help\n\n";
        std::cout << "EXAMPLES:\n";
        std::cout << "  temporis-ggg game.dot                 # Solve reachability game\n";
        std::cout << "  temporis-ggg --verbose game.dot       # Detailed output\n";
        std::cout << "  temporis-ggg -t 100 game.dot          # Custom time bound\n";
    }
    
    void output_solution(const ggg::solvers::RSSolution<ggg::graphs::GGGTemporalGraph>& solution, bool verbose) {
        std::cout << "\n=== Solution ===\n";
        std::cout << "Status: " << (solution.is_solved() ? "Solved" : "Unsolved") << std::endl;
        std::cout << "Valid: " << (solution.is_valid() ? "Yes" : "No") << std::endl;
        
        if (verbose && solution.is_solved()) {
            std::cout << "\nWinning Regions:\n";
            
            auto [vertex_begin, vertex_end] = boost::vertices(*manager_->graph());
            for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
                auto vertex = *vertex_it;
                const auto& props = (*manager_->graph())[vertex];
                
                std::cout << "  " << props.name << ": ";
                if (solution.is_won_by_player0(vertex)) {
                    std::cout << "Player 0";
                    if (solution.has_strategy(vertex)) {
                        auto strategy_vertex = solution.get_strategy(vertex);
                        if (strategy_vertex != boost::graph_traits<ggg::graphs::PresburgerTemporalGraph>::null_vertex()) {
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
};

int main(int argc, char* argv[]) {
    TemporalReachabilityExecutor executor;
    return executor.run(argc, argv);
}
