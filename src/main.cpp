#include "libggg/libggg.hpp"
#include "temporis/presburger_term.hpp"
#include "temporis/presburger_formula.hpp"
#include "temporis/temporal_game_manager.hpp"
#include "temporis/dot_parser.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <functional>
#include <memory>
#include <string>

int main(int argc, char* argv[]) {
    using namespace ggg::graphs;
    
    if (argc > 1) {
        std::string filename = argv[1];
        std::cout << "Loading Presburger Arithmetic Temporal Game from: " << filename << "\n";
        std::cout << "==================================================\n\n";
        
        // Create game manager and parser
        PresburgerTemporalGameManager manager;
        PresburgerTemporalDotParser parser;
        
        // Parse the DOT file
        if (!parser.parse_file(filename, manager)) {
            std::cerr << "Failed to parse file: " << filename << std::endl;
            return 1;
        }
        
        auto vertex_count = boost::num_vertices(manager.graph());
        auto edge_count = boost::num_edges(manager.graph());
        
        std::cout << "Presburger temporal game loaded with " << vertex_count 
                  << " vertices and " << edge_count << " edges.\n\n";
        
        // Print game structure
        std::cout << "=== Game Structure ===\n";
        std::cout << "Player 0 vertices: ";
        auto player0_vertices = manager.get_player_vertices(0);
        for (auto v : player0_vertices) {
            std::cout << manager.graph()[v].name << " ";
        }
        std::cout << "\nPlayer 1 vertices: ";
        auto player1_vertices = manager.get_player_vertices(1);
        for (auto v : player1_vertices) {
            std::cout << manager.graph()[v].name << " ";
        }
        std::cout << "\n\n";
        
        // Print formula explanations
        manager.print_formula_explanations();
        
        // Analyze edge availability over time for the loaded game
        std::cout << "=== Temporal Edge Analysis ===\n";
        for (int time = 0; time <= 25; ++time) {
            manager.advance_time(time);
            std::cout << "Time " << time << ":\n";
            
            auto [edges_begin, edges_end] = boost::edges(manager.graph());
            for (auto edge_it = edges_begin; edge_it != edges_end; ++edge_it) {
                auto source = boost::source(*edge_it, manager.graph());
                auto target = boost::target(*edge_it, manager.graph());
                bool is_active = manager.is_edge_constraint_satisfied(*edge_it, time);
                
                std::cout << "  " << manager.graph()[source].name << " -> " 
                          << manager.graph()[target].name << " (" 
                          << manager.graph()[*edge_it].label << "): " 
                          << (is_active ? "ACTIVE" : "INACTIVE") << "\n";
            }
            std::cout << "\n";
        }
        
        return 0;
    }
    
    std::cout << "Modular Temporis Demo\n";
    std::cout << "=====================\n\n";
    
    // Create game manager
    PresburgerTemporalGameManager manager;
    
    // Add some vertices and edges to test the modular structure
    auto v0 = manager.add_vertex("start", 0);
    auto v1 = manager.add_vertex("middle", 1);
    auto v2 = manager.add_vertex("end", 0);
    
    auto e1 = manager.add_edge(v0, v1, "early");
    auto e2 = manager.add_edge(v1, v2, "late");
    
    // Test PresburgerTerm
    PresburgerTerm time_term("time");
    PresburgerTerm constant_term(3);
    PresburgerTerm time_coeff("time", 2);
    
    std::cout << "PresburgerTerm tests:\n";
    std::cout << "time: " << time_term.to_string() << "\n";
    std::cout << "constant 3: " << constant_term.to_string() << "\n";
    std::cout << "2*time: " << time_coeff.to_string() << "\n";
    std::cout << "time + 3: " << (time_term + constant_term).to_string() << "\n\n";
    
    // Test PresburgerFormula (simplified without create methods)
    std::cout << "PresburgerFormula modular test: Successfully included header!\n\n";
    
    // Print game structure
    std::cout << "Game Structure:\n";
    std::cout << "Player 0 vertices: ";
    auto player0_vertices = manager.get_player_vertices(0);
    for (auto v : player0_vertices) {
        std::cout << manager.graph()[v].name << " ";
    }
    std::cout << "\nPlayer 1 vertices: ";
    auto player1_vertices = manager.get_player_vertices(1);
    for (auto v : player1_vertices) {
        std::cout << manager.graph()[v].name << " ";
    }
    std::cout << "\n\nEdges:\n";
    
    auto [edges_begin, edges_end] = boost::edges(manager.graph());
    for (auto edge_it = edges_begin; edge_it != edges_end; ++edge_it) {
        auto source = boost::source(*edge_it, manager.graph());
        auto target = boost::target(*edge_it, manager.graph());
        
        std::cout << "  " << manager.graph()[source].name << " -> " 
                  << manager.graph()[target].name << " (" 
                  << manager.graph()[*edge_it].label << ")\n";
    }
    
    std::cout << "\nModularization successful! All classes compiled and linked properly.\n";
    
    return 0;
}
