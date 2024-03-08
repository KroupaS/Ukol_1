#include <stddef.h>
#ifndef BOARD_H
#include "board.h"
#endif

#ifndef SOLVE_H
#define SOLVE_H

struct NodeState* solve(Board* board);
void sequential_solve_recurse(Board* board, NodeState* current_node, NodeState* current_best);
void solve_recurse(Board* board, NodeState* current_node, NodeState* current_best);

#endif