#include "temporal_analyzer.hpp"
#include <boost/graph/graph_traits.hpp>

TemporalAnalyzer::TemporalAnalyzer(PresburgerTemporalGameManager& manager) 
    : manager_(manager) {
}

void TemporalAnalyzer::print_game_structure() const {
    std::cout << "=== Game Structure ===\n";
    std::cout << "Player 0 vertices: ";
    auto player0_vertices = manager_.get_player_vertices(0);
    for (auto v : player0_vertices) {
        std::cout << manager_.graph()[v].name << " ";
    }
    std::cout << "\nPlayer 1 vertices: ";
    auto player1_vertices = manager_.get_player_vertices(1);
    for (auto v : player1_vertices) {
        std::cout << manager_.graph()[v].name << " ";
    }
    std::cout << "\n\n";
}

void TemporalAnalyzer::analyze_temporal_edges(int start_time, int end_time) const {
    std::cout << "=== Temporal Edge Analysis ===\n";
    for (int time = start_time; time <= end_time; ++time) {
        manager_.advance_time(time);
        std::cout << "Time " << time << ":\n";
        
        auto [edges_begin, edges_end] = boost::edges(manager_.graph());
        for (auto edge_it = edges_begin; edge_it != edges_end; ++edge_it) {
            auto source = boost::source(*edge_it, manager_.graph());
            auto target = boost::target(*edge_it, manager_.graph());
            bool is_active = manager_.is_edge_constraint_satisfied(*edge_it, time);
            
            std::cout << "  " << manager_.graph()[source].name << " -> " 
                      << manager_.graph()[target].name << " (" 
                      << manager_.graph()[*edge_it].label << "): " 
                      << (is_active ? "ACTIVE" : "INACTIVE") << "\n";
        }
        std::cout << "\n";
    }
}

void TemporalAnalyzer::generate_full_report(int start_time, int end_time) const {
    print_game_statistics();
    print_game_structure();
    manager_.print_formula_explanations();
    analyze_temporal_edges(start_time, end_time);
}

void TemporalAnalyzer::print_game_statistics() const {
    auto vertex_count = boost::num_vertices(manager_.graph());
    auto edge_count = boost::num_edges(manager_.graph());
    
    std::cout << "Presburger temporal game loaded with " << vertex_count 
              << " vertices and " << edge_count << " edges.\n\n";
}
