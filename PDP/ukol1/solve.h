#include "mpi.h"

#ifndef BOARD_H
#include "board.h"
#endif

#ifndef SOLVE_H
#define SOLVE_H

//typedef struct msg_master_to_slave {

//} msg_master_to_slave;

//NodeState* solve(Board* board, int* my_rank, int* process_count);
void solve_slave(int* my_rank, int* process_count);
NodeState* solve_master(Board* board, int* process_count);

#endif