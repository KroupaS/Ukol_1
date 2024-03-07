#include "solve.h"
#include "board.h"

void solve_recurse(Board* board, NodeState* current_node, NodeState* current_best, size_t* counter);

NodeState* solve(Board* board, size_t* recursion_counter) {
    NodeState* best_solution = (NodeState*)malloc(sizeof(NodeState));
    uint upper_bound = board->upper_bound;
    best_solution->depth = upper_bound;
    NodeState* initial_state = initFirstNode(board);
    uint lower_bound = getLowerBound(board, initial_state);
    board->lower_bound = lower_bound;

    printf("Initial upper bound = %u\n", upper_bound);
    printf("Initial lower bound = %u\n", lower_bound);

    solve_recurse(board, initial_state, best_solution, recursion_counter);
    // Probably shouldnt free initial state, should be freed in solve_recurse
    //NodeDestructor(initial_state);

    return best_solution;
}

void solve_recurse(Board* board, NodeState* current_node, NodeState* current_best, size_t* counter) {
    (*counter)++;
    if ((*counter) % 100000000 == 0){
        printf("Recursion count = %zu\n", *counter);
    }
    if (current_node->depth >= board->upper_bound) {
        //printf("The dwarves delved too deep\n");
        NodeDestructor(current_node);
        return;
    }

    //if(current_node->depth > 4) {
        //printf("DEBUG ending recursion at depth 4 with error \n");
        //NodeDestructor(current_node);
    //}

    if ((current_node->unfinished_black == 0) && (current_node->unfinished_white == 0)) {
        //printf("Finished solution\n");
        if (current_node->depth <= board->lower_bound) {
            printf("Solution matches lower bound, ending\n");
            CopyNodeIntoNode(board, current_node, current_best);
            free(current_node);
            // TODO check that this works as intended, should free the top level struct but nothing from <current_best> (These should be two identical structs at different addresses)
            // Optimal solution, end early by setting upper bound of the board to 0 - every recursive call should exit immediatelly
            board->upper_bound = 0;
            return;
        } else {
            //printf("Valid solution\n");
            //printf("Current solution depth: %i\n", current_node->depth);
            // Solution - compare & update best but dont quit early
            
            if (current_node->depth < current_best->depth) {
                CopyNodeIntoNode(board, current_node, current_best);
                board->upper_bound = current_best->depth;
                free(current_node);
                return;
            } else {
                NodeDestructor(current_node);
                return;
            }
        }
    } else {
        // Unfinished, continue and recurse
        GetAvailableMoves(board, current_node);
        //PrintNode(current_node);
        if (current_node->available_moves.Count > 0){
            for (int i = 0; i < current_node->available_moves.Count; i++) {
                if ((current_node->depth + current_node->available_moves.MovesAndLowerBounds[i].lower_bound+1 ) < board->upper_bound) {
                    NodeState* new_node = CopyNode(board, current_node);
                    NodeMakeMove(board, new_node, current_node->available_moves.MovesAndLowerBounds[i].move);
                    solve_recurse(board, new_node, current_best, counter);
                } else {
                    //printf("Higher than current upper bound\n");
                }
            }
            NodeDestructor(current_node);
        } else {
            // Its possible some pawns are blocked in by pawns that cant move anymore
            NodeDestructor(current_node);
            return;
        }

    }

    // check if state is invalid (depth >= upper bound)
    // check if state is a valid solution -> update upper bound etc
    // If solution matches board->lower_bound, end program
    // get available moves
    // recurse
    // NodeDestructor






}