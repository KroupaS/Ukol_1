#include "solve.h"

#define MAX_MASTER_MSG_COUNT (1000)
#define HIGH_MASTER_MSG_COUNT (64)
#define MAX_SLAVE_STATE_COUNT (100000)
#define MAX_SLAVE_PROC_COUNT (64)

void solve_recurse_master(Board* board, NodeState* current_node, NodeState* current_best, NodeState** state_queue, uint* state_counter);
void solve_recurse_slave(Board* board, NodeState* current_node, NodeState* current_best, NodeState** state_queue, uint* state_counter);
void solve_recurse_parallel(Board* board, NodeState* current_node, NodeState* current_best);
int SlavesUnfinished(int* slave_count, int* available_slaves);

NodeState* solve_master(Board* board, int* process_count, const int verbosity) {
    MPI_Status status;
    int dest;
    int tag = 1;
    int available_slaves[MAX_SLAVE_PROC_COUNT];         // Element 0 of this array is unused. Element x == 1 -> slave with rank x is working, x == 0 -> slave with rank x is available
    int slave_count = (*process_count) - 1;
    int *ptr_slave_count = &slave_count;

    // Solve sequentially until depth 2, then just send and receive messages
    
    NodeState* best_solution = initBestSolution(board);
    best_solution->depth = board->upper_bound;
    NodeState* initial_state = initFirstNode(board);
    
    uint lower_bound = getLowerBound(board, initial_state);
    board->lower_bound = lower_bound;
    
    NodeState** state_queue = (NodeState**)malloc(sizeof(NodeState*) * MAX_SLAVE_STATE_COUNT);
    uint state_counter = 0;
    uint* ptr_counter = &state_counter;
    uint job_count = 0;

    solve_recurse_master(board, initial_state, best_solution, state_queue, ptr_counter);
    if (verbosity == 1) {
        printf("Master: state queue size = %u\n", state_counter); fflush(stdout);
    }

    if (slave_count < 1) {
        // Edge case no slaves
        printf("Master: ERROR Process count is 1, no slaves\n"); fflush(stdout);
        return best_solution;
    }

    if (slave_count > state_counter) {
        // Edge case where we have more slaves than tasks for them
        printf("Master: ERROR too many slaves too few tasks, reconfigure parameters!\n"); fflush(stdout);
        return best_solution;
    }

    if ((*process_count) > MAX_SLAVE_PROC_COUNT) {
        printf("Master: ERROR too many slaves, current maximum is %d\n", MAX_SLAVE_PROC_COUNT - 1); fflush(stdout);
        return best_solution;
    }
    
    NodeStateSerial task_msg;
    NodeStateSerial received_msg;
    memset(available_slaves, 0, MAX_SLAVE_PROC_COUNT * sizeof(int));
    // First lets send one task to each slave
    for (int i=0; i<slave_count; i++) {
        SerializeNodeState(&task_msg, board, state_queue[job_count]);
        NodeDestructor(state_queue[job_count]);
        job_count++;        
        dest = i + 1;
    	if (verbosity == 1) {
            printf("Master: sending job %d to Slave %d\n", job_count-1, dest); fflush(stdout);
	}
        MPI_Ssend(&task_msg, sizeof(NodeStateSerial), MPI_PACKED, dest, tag, MPI_COMM_WORLD);
        available_slaves[dest] = 1; 
    }
    
    while (job_count < state_counter) {
        // If job_count < state_counter, we didnt send out all states as jobs yet.

        // Wait for a message from finished slave because some jobs must be unfinished
        MPI_Recv(&received_msg, sizeof(NodeStateSerial), MPI_PACKED, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        //NodeState* slave_output = DeSerializeSolution(&received_msg);
        if (received_msg.depth < best_solution->depth) {
            UpdateSolutionFromSerial(&received_msg, best_solution);
        }

        // Send another job to the same slave 
        // Set slave board upper bound for pruning - might not work
        Board slave_board = *board;
        slave_board.upper_bound = best_solution->depth;
        SerializeNodeState(&task_msg, &slave_board, state_queue[job_count]);
        NodeDestructor(state_queue[job_count]);
        job_count++;        
        int slave_rank = status.MPI_SOURCE;
    	if (verbosity == 1) {
            printf("Master: Received finished message from Slave %d, sending new job %d\n", slave_rank, job_count-1); fflush(stdout);
	}
        MPI_Ssend(&task_msg, sizeof(NodeStateSerial), MPI_PACKED, slave_rank, tag, MPI_COMM_WORLD);
    }
    
    // job_count == state_counter now, so all states have been handed out as tasks 
    // Wait until all workers send final outputs
    tag = 0;    // Set tag to 0 -> kill
    if (verbosity == 1) {
        printf("Master: All jobs served, killing slaves!\n"); fflush(stdout);
    }
    while (SlavesUnfinished(ptr_slave_count, available_slaves)) {
        MPI_Recv(&received_msg, sizeof(NodeStateSerial), MPI_PACKED, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (received_msg.depth < best_solution->depth) {
            UpdateSolutionFromSerial(&received_msg, best_solution);
        }
        int slave_rank = status.MPI_SOURCE;
        if (verbosity == 1) {
            printf("Master: Received finished message from Slave %d, sending kill message\n", slave_rank); fflush(stdout);
        }
        MPI_Ssend(&task_msg, sizeof(NodeStateSerial), MPI_PACKED, slave_rank, tag, MPI_COMM_WORLD);
        available_slaves[slave_rank] = 0;
    }
    
    // Finish up 
    return best_solution;
}
    

void solve_recurse_master(Board* board, NodeState* current_node, NodeState* current_best, NodeState** state_queue, uint* state_counter) {
    if ((*state_counter) > HIGH_MASTER_MSG_COUNT) {
        // Too many messages for slaves were generated already, dont generate moves for this state, just add it to the queue
        // Have to copy the node, because parent will call NodeDestructor eventually
        NodeState* new_node = CopyNode(board, current_node);
        state_queue[*state_counter] = new_node;
        (*state_counter)++;
        if ((*state_counter) >= MAX_MASTER_MSG_COUNT) {
            printf("ERROR MASTER MAX STATE COUNT TOO LOW\n");
            return;
        }
    } else {
        // Unfinished, continue and recurse
        GetAvailableMoves(board, current_node);
        if (current_node->available_moves.Count > 0){
            if (current_node->depth < 2) {
                for (int i = 0; i < current_node->available_moves.Count; i++) {
                    if ((current_node->depth + current_node->available_moves.MovesAndLowerBounds[i].lower_bound + 1) < current_best->depth) {
                        // Check that S + d(S') + 1 < upper_bound, so recurse on these moves
                        NodeState* new_node = CopyNode(board, current_node);
                        NodeMakeMove(board, new_node, current_node->available_moves.MovesAndLowerBounds[i].move);
                        solve_recurse_master(board, new_node, current_best, state_queue, state_counter);
                        NodeDestructor(new_node);
                    }
                }
            }
            else {
                // Add to the queue if depth >= 2 and return
                for (int i = 0; i < current_node->available_moves.Count; i++) {
                    if ((current_node->depth + current_node->available_moves.MovesAndLowerBounds[i].lower_bound + 1) < current_best->depth) {
                        NodeState* new_node = CopyNode(board, current_node);
                        NodeMakeMove(board, new_node, current_node->available_moves.MovesAndLowerBounds[i].move);
                        //solve_recurse(board, new_node, current_best);
                        //NodeDestructor(new_node);
                        state_queue[*state_counter] = new_node;
                        (*state_counter)++;
                        if ((*state_counter) >= MAX_MASTER_MSG_COUNT) {
                            printf("ERROR MASTER MAX STATE COUNT TOO LOW\n");
                            return;
                        }
                    }
                }
            }
        }
    }
    return;
}

    
void solve_slave(int* my_rank, int* process_count, const int verbosity) {
    MPI_Status status;
    NodeStateSerial received_msg;
    NodeStateSerial sent_msg;
    NodeState** state_queue = (NodeState**)malloc(sizeof(NodeState*) * MAX_SLAVE_STATE_COUNT);

    while (1){
        MPI_Recv(&received_msg, sizeof(NodeStateSerial), MPI_PACKED, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (verbosity == 1) {
            printf("Slave %d: Received job from master, working...\n", *my_rank); fflush(stdout);
	}
        if (status.MPI_TAG == 1) {
            // Work time
            uint state_counter = 0;
            uint* ptr_counter = &state_counter;
            Board slave_board;

            NodeState* slave_initial_state = DeSerializeNodeState(&slave_board, &received_msg);
            NodeState* slave_best_solution = initBestSolution(&slave_board);
            slave_best_solution->depth = slave_board.upper_bound;

            // Solve sequentially until depth 3, then add all new states to the queue
            solve_recurse_slave(&slave_board, slave_initial_state, slave_best_solution, state_queue, ptr_counter);
        
            // Queue is ready, solve states from queue in parallel loop
            #pragma omp parallel for schedule(dynamic)
            for (int i=0; i<state_counter; i++) {
                solve_recurse_parallel(&slave_board, state_queue[i], slave_best_solution);
                NodeDestructor(state_queue[i]);
            }

            SerializeSolution(&sent_msg, slave_best_solution);
            if (verbosity == 1) {
                printf("Slave %d: finished job, sending to master\n", *my_rank); fflush(stdout);
	    }
            MPI_Ssend(&sent_msg, sizeof(NodeStateSerial), MPI_PACKED, 0, 1, MPI_COMM_WORLD);
            if (verbosity == 1) {
                printf("Slave %d: master received finished job, waiting for next message\n", *my_rank); fflush(stdout);
	    }
            
            NodeDestructor(slave_initial_state);
            FreeBestSolution(slave_best_solution);
        } else if (status.MPI_TAG == 0) {
            if (verbosity == 1) {
                printf("Slave %d: received kill message, ending...\n", *my_rank); fflush(stdout);
	    }
            // Clean up and exit
            free(state_queue);
            return;
        } else {
            printf("Slave %d: ERRORunknown tag %d\n", *my_rank, status.MPI_TAG); fflush(stdout);
            return;
        }
    }
    

}


void solve_recurse_slave(Board* board, NodeState* current_node, NodeState* current_best, NodeState** state_queue, uint* state_counter) {
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
            if (current_node->depth < 3) {
                for (int i = 0; i < current_node->available_moves.Count; i++) {
                    if ((current_node->depth + current_node->available_moves.MovesAndLowerBounds[i].lower_bound + 1) < current_best->depth) {
                        // Check that S + d(S') + 1 < upper_bound, so recurse on these moves
                        NodeState* new_node = CopyNode(board, current_node);
                        NodeMakeMove(board, new_node, current_node->available_moves.MovesAndLowerBounds[i].move);
                        solve_recurse_slave(board, new_node, current_best, state_queue, state_counter);
                        NodeDestructor(new_node);
                    }
                }
            }
            else {
                // Add to the queue if depth >= 3 and return
                for (int i = 0; i < current_node->available_moves.Count; i++) {
                    if ((current_node->depth + current_node->available_moves.MovesAndLowerBounds[i].lower_bound + 1) < current_best->depth) {
                        NodeState* new_node = CopyNode(board, current_node);
                        NodeMakeMove(board, new_node, current_node->available_moves.MovesAndLowerBounds[i].move);
                        state_queue[*state_counter] = new_node;
                        (*state_counter)++;
                        if ((*state_counter) >= MAX_SLAVE_STATE_COUNT) {
                            printf("SLAVE ERROR MAX STATE COUNT TOO LOW\n"); fflush(stdout);
                            return;
                        }
                    }
                }
            }
        }
    }
    return;
}


void solve_recurse_parallel(Board* board, NodeState* current_node, NodeState* current_best) {
    // Slaves will use this in for loop
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

int SlavesUnfinished(int* slave_count, int* available_slaves) {
    int unfinished = 0;
    for (int i=1; i<(*slave_count+1); i++) {
        if (available_slaves[i] == 1) {
            unfinished = 1;
        }
    }
    return unfinished;
}
