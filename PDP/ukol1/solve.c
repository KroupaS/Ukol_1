#include "solve.h"
#include "board.h"

void solve_recurse(Board* board, NodeState* current_node, NodeState* current_best);

NodeState* solve(Board* board) {
    uint upper_bound = board->upper_bound;
    NodeState* best_solution = initBestSolution(board);
    best_solution->depth = upper_bound;
    NodeState* initial_state = initFirstNode(board);

    uint lower_bound = getLowerBound(board, initial_state);
    board->lower_bound = lower_bound;

    //printf("Initial upper bound = %u\n", upper_bound);
    //printf("Initial lower bound = %u\n", lower_bound);

    solve_recurse(board, initial_state, best_solution);
    return best_solution;
}

void solve_recurse(Board* board, NodeState* current_node, NodeState* current_best) {
    if (current_node->depth >= current_best->depth) {
        // Exit if depth exceeds upper bound
        return;
    }

    if ((current_node->unfinished_black == 0) && (current_node->unfinished_white == 0)) {
        // If all white pawns are in B and vice versa, update best solution
        if (current_node->depth < current_best->depth) {
            CopyMoves(current_node, current_best);
        }
    } else {
        // Unfinished, continue and recurse
        GetAvailableMoves(board, current_node);
        if (current_node->available_moves.Count > 0){
            for (int i = 0; i < current_node->available_moves.Count; i++) {
                if ((current_node->depth + current_node->available_moves.MovesAndLowerBounds[i].lower_bound + 1) < current_best->depth) {
                    // Check that S + d(S') + 1 < upper_bound, so recurse on these moves
                    NodeState* new_node = CopyNode(board, current_node);
                    NodeMakeMove(board, new_node, current_node->available_moves.MovesAndLowerBounds[i].move);
                    solve_recurse(board, new_node, current_best);
                    NodeDestructor(new_node);
                }
            }
        }
    }
    return;
}
