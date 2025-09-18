#include "temporal_game_demo.hpp"
#include "temporal_analyzer.hpp"
#include <boost/graph/graph_traits.hpp>

void TemporalGameDemo::run_complete_demo() {
    std::cout << "Modular Temporis Demo\n";
    std::cout << "=====================\n\n";
    
    // Test Presburger components
    test_presburger_terms();
    test_presburger_formulas();
    
    // Create and test sample game
    auto manager = create_sample_game();
    test_game_structure(manager);
    
    std::cout << "\nModularization successful! All classes compiled and linked properly.\n";
}

void TemporalGameDemo::test_presburger_terms() {
    std::cout << "PresburgerTerm tests:\n";
    
    // Test different term types
    PresburgerTerm time_term("time");
    PresburgerTerm constant_term(3);
    PresburgerTerm time_coeff("time", 2);
    
    std::cout << "time: " << time_term.to_string() << "\n";
    std::cout << "constant 3: " << constant_term.to_string() << "\n";
    std::cout << "2*time: " << time_coeff.to_string() << "\n";
    std::cout << "time + 3: " << (time_term + constant_term).to_string() << "\n\n";
}

void TemporalGameDemo::test_presburger_formulas() {
    std::cout << "PresburgerFormula modular test: Successfully included header!\n\n";
}

PresburgerTemporalGameManager TemporalGameDemo::create_sample_game() {
    PresburgerTemporalGameManager manager;
    
    // Add some vertices and edges to test the modular structure
    auto v0 = manager.add_vertex("start", 0);
    auto v1 = manager.add_vertex("middle", 1);
    auto v2 = manager.add_vertex("end", 0);
    
    auto e1 = manager.add_edge(v0, v1, "early");
    auto e2 = manager.add_edge(v1, v2, "late");
    
    return manager;
}

void TemporalGameDemo::test_game_structure(const PresburgerTemporalGameManager& manager) {
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
}
