#include "static_expansion_solver.hpp"
#include "ggg_temporal_graph.hpp"
#include "libggg/utils/solver_wrapper.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

// Simple logging helpers for temporis
namespace {
    bool g_verbose = false;
    bool g_debug = false;
    
    template<typename... Args>
    void log_error(Args... args) {
        std::cerr << "[ERROR] ";
        ((std::cerr << args), ...);
        std::cerr << std::endl;
    }
    
    template<typename... Args>
    void log_info(Args... args) {
        std::cout << "[INFO] ";
        ((std::cout << args), ...);
        std::cout << std::endl;
    }
    
    template<typename... Args>
    void log_debug(Args... args) {
        if (g_debug) {
            std::cout << "[DEBUG] ";
            ((std::cout << args), ...);
            std::cout << std::endl;
        }
    }
}

/**
 * @brief Multi-algorithm temporal solver executable with both backwards and static expansion
 */
class StaticExpansionTemporalExecutor {
private:
    std::shared_ptr<ggg::graphs::GGGTemporalGameManager> manager_;
    std::shared_ptr<ggg::graphs::GGGReachabilityObjective> objective_;
    int time_bound_;
    bool verbose_;
    bool debug_;
    bool csv_output_;
    bool time_only_;
    bool validate_;

public:
    StaticExpansionTemporalExecutor() 
        : time_bound_(50), verbose_(false), debug_(false), 
          csv_output_(false), time_only_(false), validate_(false) {}

    bool parse_arguments(int argc, char* argv[]) {
        std::vector<std::string> files;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--help" || arg == "-h") {
                print_usage();
                return false;
            } else if (arg == "--verbose" || arg == "-v") {
                verbose_ = true;
                g_verbose = true;
                log_debug("Verbose mode enabled");
            } else if (arg == "--debug") {
                debug_ = true;
                g_debug = true;
                verbose_ = true;
                g_verbose = true;
                log_debug("Debug mode enabled");
            } else if (arg == "--validate") {
                validate_ = true;
                log_info("Validation mode enabled");
            } else if (arg == "--csv") {
                csv_output_ = true;
            } else if (arg == "--time-only") {
                time_only_ = true;
            } else if (arg == "--time-bound") {
                if (i + 1 < argc) {
                    try {
                        time_bound_ = std::stoi(argv[++i]);
                    } catch (const std::exception&) {
                        log_error("Invalid time bound value: ", argv[i]);
                        return false;
                    }
                } else {
                    log_error("--time-bound requires a value");
                    return false;
                }
            } else if (arg.empty() || arg[0] == '-') {
                log_error("Unknown option: ", arg);
                return false;
            } else {
                files.push_back(arg);
            }
        }
        
        // Handle input - either from file or stdin
        if (files.empty()) {
            return parse_from_stdin();
        } else if (files.size() == 1) {
            return parse_from_file(files[0]);
        } else {
            log_error("Only one input file allowed");
            return false;
        }
    }

    bool parse_from_stdin() {
        std::string content;
        std::string line;
        while (std::getline(std::cin, line)) {
            content += line + "\n";
        }
        
        int extracted_time_bound = extract_time_bound_from_content(content);
        if (extracted_time_bound > 0) {
            time_bound_ = extracted_time_bound;
            if (verbose_) {
                log_info("Extracted time bound from input: ", time_bound_);
            }
        }
        
        std::istringstream stream(content);
        return parse_graph(stream, "stdin");
    }

    bool parse_from_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            log_error("Could not open file: ", filename);
            return false;
        }
        
        // Extract time bound from file content
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        int extracted_time_bound = extract_time_bound_from_content(content);
        if (extracted_time_bound > 0) {
            time_bound_ = extracted_time_bound;
            if (verbose_) {
                log_info("Extracted time bound from file: ", time_bound_);
            }
        }
        
        // Parse graph from content
        std::istringstream stream(content);
        return parse_graph(stream, filename);
    }

    int extract_time_bound_from_content(const std::string& content) {
        std::istringstream stream(content);
        std::string line;
        
        while (std::getline(stream, line)) {
            // Look for comment lines with time_bound
            if (line.find("// time_bound:") != std::string::npos || 
                line.find("//time_bound:") != std::string::npos) {
                
                // Extract the number after the colon
                size_t colon_pos = line.find(':');
                if (colon_pos != std::string::npos) {
                    std::string time_str = line.substr(colon_pos + 1);
                    // Remove whitespace
                    time_str.erase(0, time_str.find_first_not_of(" \t"));
                    time_str.erase(time_str.find_last_not_of(" \t\n\r") + 1);
                    
                    try {
                        return std::stoi(time_str);
                    } catch (const std::exception&) {
                        log_error("Could not parse time bound: ", time_str);
                    }
                }
            }
        }
        
        return -1; // Not found
    }

    bool parse_graph(std::istream& input, const std::string& source_name) {
        try {
            manager_ = std::make_shared<ggg::graphs::GGGTemporalGameManager>();
            
            // Read entire input into string
            std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
            
            if (!manager_->load_from_dot_string(content)) {
                log_error("Failed to parse temporal graph from ", source_name);
                return false;
            }
            
            log_debug("Successfully parsed graph with ", 
                      boost::num_vertices(*manager_->graph()), " vertices");
            
            // Create objective
            std::set<ggg::graphs::GGGTemporalGraph::vertex_descriptor> targets;
            targets = manager_->get_target_vertices();
            
            objective_ = std::make_shared<ggg::graphs::GGGReachabilityObjective>(
                ggg::graphs::GGGReachabilityObjective::Type::REACHABILITY, targets);
            
            return true;
        } catch (const std::exception& e) {
            log_error("Exception during parsing: ", e.what());
            return false;
        }
    }

    void solve_and_output() {
        if (!manager_ || !objective_) {
            log_error("Graph not properly initialized");
            return;
        }
        
        // Only show solver info in normal output modes
        if (!csv_output_ && !time_only_) {
            std::cout << "Algorithm: Static Expansion" << std::endl;
            std::cout << "Time bound: " << time_bound_ << std::endl;
        }
        
        // Create static expansion solver
        auto solver = std::make_unique<ggg::solvers::StaticExpansionSolver>(
            manager_, objective_, time_bound_, verbose_);
        
        // Solve the game
        auto start_time = std::chrono::high_resolution_clock::now();
        auto solution = solver->solve(*manager_->graph());
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto solve_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        double solve_time_seconds = solve_duration.count() / 1000000.0;
        
        // Handle different output modes
        if (time_only_) {
            std::cout << std::fixed << std::setprecision(6) << solve_time_seconds << std::endl;
            return;
        }
        
        if (csv_output_) {
            // Output CSV format with static expansion statistics
            auto* static_solver = static_cast<ggg::solvers::StaticExpansionSolver*>(solver.get());
            const auto& stats = static_solver->get_statistics();
            std::string extra_stats = std::to_string(stats.expanded_vertices) + "," + 
                                     std::to_string(stats.expanded_edges) + "," +
                                     std::to_string(stats.attractor_vertices);
            
            std::cout << solver->get_name() << ","
                      << "game" << ","
                      << "solved" << ","
                      << std::fixed << std::setprecision(6) << solve_time_seconds << ","
                      << extra_stats << std::endl;
            return;
        }
        
        // Standard output mode
        std::cout << "\nSolver: " << solver->get_name() << std::endl;
        std::cout << "Solve time: " << std::fixed << std::setprecision(6) << solve_time_seconds << "s" << std::endl;
        
        // Print static expansion statistics
        if (verbose_) {
            auto* static_solver = static_cast<ggg::solvers::StaticExpansionSolver*>(solver.get());
            const auto& stats = static_solver->get_statistics();
            
            std::cout << "\n=== Static Expansion Statistics ===" << std::endl;
            std::cout << "Original graph: " << stats.original_vertices << " vertices, " << stats.original_edges << " edges" << std::endl;
            std::cout << "Expanded graph: " << stats.expanded_vertices << " vertices, " << stats.expanded_edges << " edges" << std::endl;
            std::cout << "Time layers: " << stats.time_layers << std::endl;
            std::cout << "Expansion time: " << stats.expansion_time.count() << "s" << std::endl;
            std::cout << "Attractor time: " << stats.attractor_time.count() << "s" << std::endl;
            std::cout << "Constraint evaluations: " << stats.constraint_evaluations << std::endl;
        }
        
        std::cout << "\n=== Solution ===" << std::endl;
        std::cout << "Status: Solved" << std::endl;
        
        // Output winning regions and strategies
        std::cout << "\nWinning Regions:" << std::endl;
        auto [vertex_begin, vertex_end] = boost::vertices(*manager_->graph());
        for (auto vertex_it = vertex_begin; vertex_it != vertex_end; ++vertex_it) {
            auto vertex = *vertex_it;
            int winning_player = solution.get_winning_player(vertex);
            std::cout << "  " << (*manager_->graph())[vertex].name << ": Player " << winning_player;
            
            if (verbose_ && solution.has_strategy(vertex)) {
                auto strategy_vertex = solution.get_strategy(vertex);
                if (strategy_vertex != boost::graph_traits<ggg::graphs::GGGTemporalGraph>::null_vertex()) {
                    std::cout << " -> " << (*manager_->graph())[strategy_vertex].name;
                }
            }
            std::cout << std::endl;
        }
    }

    void print_usage() {
        std::cout << "Static Expansion Temporal Reachability Solver\n\n";
        std::cout << "USAGE:\n";
        std::cout << "  temporis_static_expansion [OPTIONS] [input_file.dot]       # Read from file\n";
        std::cout << "  temporis_static_expansion [OPTIONS] < input_file.dot       # Read from stdin\n\n";
        std::cout << "OPTIONS:\n";
        std::cout << "  -h, --help              Show this help message\n";
        std::cout << "  -v, --verbose           Enable verbose output\n";
        std::cout << "  --debug                 Enable debug output\n";
        std::cout << "  --validate              Enable solution validation\n";
        std::cout << "  --csv                   Output in CSV format for benchmarking\n";
        std::cout << "  --time-only             Output only solve time in seconds\n";
        std::cout << "  --time-bound TIME       Set time bound (default: 50)\n\n";
        std::cout << "ALGORITHM:\n";
        std::cout << "  This solver uses static expansion: creates (vertex,time) pairs for all time layers,\n";
        std::cout << "  then uses GGG's attractor computation on the expanded graph.\n\n";
        std::cout << "EXAMPLES:\n";
        std::cout << "  temporis_static_expansion game.dot\n";
        std::cout << "  temporis_static_expansion --verbose game.dot\n";
        std::cout << "  temporis_static_expansion --time-only game.dot\n";
    }
};

int main(int argc, char* argv[]) {
    StaticExpansionTemporalExecutor executor;
    
    if (!executor.parse_arguments(argc, argv)) {
        return 1;
    }
    
    executor.solve_and_output();
    return 0;
}
