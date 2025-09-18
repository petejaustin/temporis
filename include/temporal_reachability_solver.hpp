#pragma once

#include "temporal_game_manager.hpp"
#include "reachability_objective.hpp"
#include <map>
#include <set>
#include <vector>
#include <memory>

using namespace ggg::graphs;

/**
 * @brief Game state combining vertex and time
 */
struct GameState {
    PresburgerTemporalVertex vertex;
    int time;
    
    bool operator<(const GameState& other) const {
        if (vertex != other.vertex) return vertex < other.vertex;
        return time < other.time;
    }
    
    bool operator==(const GameState& other) const {
        return vertex == other.vertex && time == other.time;
    }
};

/**
 * @brief Represents the solution to a temporal reachability game
 */
struct ReachabilityGameSolution {
    enum class Winner { PLAYER_0, PLAYER_1, UNDETERMINED };
    
    Winner winner;
    std::map<GameState, Winner> winning_regions;  // Which states each player wins from
    std::map<GameState, PresburgerTemporalVertex> strategy; // Optimal moves
    std::string explanation;
    
    ReachabilityGameSolution() : winner(Winner::UNDETERMINED) {}
};

/**
 * @brief Solver for temporal reachability games
 * 
 * Implements algorithms to solve reachability objectives on temporal games
 * where edge availability depends on Presburger arithmetic constraints.
 */
class TemporalReachabilitySolver {
public:
    /**
     * @brief Construct a solver for the given game and objective
     * 
     * @param manager Game manager containing the temporal game
     * @param objective Reachability objective to solve
     * @param max_time Maximum time to consider (for bounded analysis)
     */
    TemporalReachabilitySolver(PresburgerTemporalGameManager& manager,
                              std::shared_ptr<ReachabilityObjective> objective,
                              int max_time = 50);

    /**
     * @brief Solve the reachability game
     * 
     * @param initial_vertex Starting vertex
     * @param initial_time Starting time (default: 0)
     * @return Solution containing winner and strategies
     */
    ReachabilityGameSolution solve(PresburgerTemporalVertex initial_vertex, int initial_time = 0);

    /**
     * @brief Compute winning regions for all vertices
     * 
     * @param initial_time Starting time for analysis
     * @return Pair of (Player 0 winning vertices, Player 1 winning vertices)
     */
    std::pair<std::set<PresburgerTemporalVertex>, std::set<PresburgerTemporalVertex>> 
    compute_winning_regions(int initial_time = 0);

    /**
     * @brief Check if a player can win from a given state
     * 
     * @param state Game state to check
     * @param player Player to check (0 or 1)
     * @return true if player can guarantee the objective from this state
     */
    bool can_win_from_state(const GameState& state, int player);

    /**
     * @brief Get available moves from a game state
     * 
     * @param state Current game state
     * @return Vector of reachable next vertices
     */
    std::vector<PresburgerTemporalVertex> get_available_moves(const GameState& state);

    /**
     * @brief Print detailed analysis of the game solution
     * 
     * @param solution Game solution to analyze
     * @param initial_state Starting state
     */
    void print_solution_analysis(const ReachabilityGameSolution& solution, 
                                const GameState& initial_state);

    /**
     * @brief Print winning regions analysis for all vertices
     * 
     * @param player0_winning Set of vertices where Player 0 can win
     * @param player1_winning Set of vertices where Player 1 can win
     */
    void print_winning_regions_analysis(const std::set<PresburgerTemporalVertex>& player0_winning,
                                       const std::set<PresburgerTemporalVertex>& player1_winning);

private:
    PresburgerTemporalGameManager& manager_;
    std::shared_ptr<ReachabilityObjective> objective_;
    int max_time_;
    
    // Memoization for dynamic programming
    std::map<GameState, ReachabilityGameSolution::Winner> memo_;
    
    /**
     * @brief Recursive solver using minimax with memoization
     */
    ReachabilityGameSolution::Winner solve_recursive(const GameState& state, std::set<GameState>& visited);
    
    /**
     * @brief Check if a state is terminal (objective satisfied/failed)
     */
    bool is_terminal_state(const GameState& state, ReachabilityGameSolution::Winner& winner);
    
    /**
     * @brief Build the complete solution from memoized results
     */
    ReachabilityGameSolution build_solution(PresburgerTemporalVertex initial_vertex, int initial_time);
};
