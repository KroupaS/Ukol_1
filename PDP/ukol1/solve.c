#include "solve.h"
#include "board.h"

#define MAX_STATE_COUNT (100000)

void solve_recurse_seq(Board* board, NodeState* current_node, NodeState* current_best, NodeState** state_queue, uint* state_counter);
void solve_recurse_parallel(Board* board, NodeState* current_node, NodeState* current_best);

NodeState* solve(Board* board) {
    uint upper_bound = board->upper_bound;
    NodeState* best_solution = initBestSolution(board);
    best_solution->depth = upper_bound;
    NodeState* initial_state = initFirstNode(board);
    
    NodeState** state_queue = (NodeState**)malloc(sizeof(NodeState*) * MAX_STATE_COUNT);
    uint state_counter = 0;
    uint* ptr_counter = &state_counter;


    uint lower_bound = getLowerBound(board, initial_state);
    board->lower_bound = lower_bound;

    //printf("Initial upper bound = %u\n", upper_bound);
    //printf("Initial lower bound = %u\n", lower_bound);

    solve_recurse_seq(board, initial_state, best_solution, state_queue, ptr_counter);
    
    // Queue is ready

    #pragma omp parallel for
    for (int i=0; i<state_counter; i++) {
        solve_recurse_parallel(board, state_queue[i], best_solution);
        NodeDestructor(state_queue[i]);
    }
    return best_solution;
}

void solve_recurse_seq(Board* board, NodeState* current_node, NodeState* current_best, NodeState** state_queue, uint* state_counter) {
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
            if (current_node->depth < 4) {
                for (int i = 0; i < current_node->available_moves.Count; i++) {
                    if ((current_node->depth + current_node->available_moves.MovesAndLowerBounds[i].lower_bound + 1) < current_best->depth) {
                        // Check that S + d(S') + 1 < upper_bound, so recurse on these moves
                        NodeState* new_node = CopyNode(board, current_node);
                        NodeMakeMove(board, new_node, current_node->available_moves.MovesAndLowerBounds[i].move);
                        solve_recurse_seq(board, new_node, current_best, state_queue, state_counter);
                        NodeDestructor(new_node);
                    }
                }
            }
            else {
                // Add to the queue if depth >= 4 and return
                for (int i = 0; i < current_node->available_moves.Count; i++) {
                    if ((current_node->depth + current_node->available_moves.MovesAndLowerBounds[i].lower_bound + 1) < current_best->depth) {
                        NodeState* new_node = CopyNode(board, current_node);
                        NodeMakeMove(board, new_node, current_node->available_moves.MovesAndLowerBounds[i].move);
                        //solve_recurse(board, new_node, current_best);
                        //NodeDestructor(new_node);
                        state_queue[*state_counter] = new_node;
                        (*state_counter)++;
                        if ((*state_counter) >= MAX_STATE_COUNT) {
                            printf("ERROR MAX STATE COUNT TOO LOW\n");
                        }
                    }
                }
            }
        }
    }
    return;
}

void solve_recurse_parallel(Board* board, NodeState* current_node, NodeState* current_best) {
    if (current_node->depth >= current_best->depth) {
        // Exit if depth exceeds upper bound
        return;
    }

    if ((current_node->unfinished_black == 0) && (current_node->unfinished_white == 0)) {
        // If all white pawns are in B and vice versa, update best solution
        if (current_node->depth < current_best->depth) {
            # pragma omp critical
            {
                if (current_node->depth < current_best->depth) {
                    CopyMoves(current_node, current_best);
                }
            }
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
                    solve_recurse_parallel(board, new_node, current_best);
                    NodeDestructor(new_node);
                }
            }
        }
    }
    return;
}