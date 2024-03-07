#include <stddef.h>
#ifndef BOARD_H
#include "board.h"
#endif

#ifndef SOLVE_H
#define SOLVE_H

struct NodeState* solve(Board* board, size_t* recursion_counter);

#endif