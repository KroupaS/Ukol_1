#ifndef BOARD_H
#define BOARD_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

typedef unsigned int uint;

#define WHITE_MOVE 0
#define BLACK_MOVE 1

// [X,Y] -> X is horizontal axis, Y is vertical axis. Origin is top left
typedef struct Point {
    char X;
    char Y;
} Point;

typedef struct Move {
    Point Source;
    Point Dest;
} Move;

typedef struct MoveAndLowerBound {
    Move move;
    uint lower_bound;  // Should be from -2 to +2 including
} MoveAndLowerBound;

// Used to store possible moves from a position, and moves that were already made
typedef struct AvailableMoves {
    uint Count;                             // Number of possible moves should be <= (400/2) * 8
    MoveAndLowerBound* MovesAndLowerBounds;
} AvailableMoves;

typedef struct Area {
    Point top_left;
    Point bot_right;
} Area;

// This struct will be initialized once, all nodes have a pointer to it
typedef struct Board {
    Area W_area;        // 4 
    Area B_area;        // 8
    uint upper_bound;   // 12
    uint lower_bound;   // 16
    uint k;             // 20 Maximum k should be <= 400/2
    char m;             // 21
    char n;             // 22
} Board;

typedef struct NodeState {
    Point* White_positions;     // Board.k size array
    Point* Black_positions;     // Board.k size array
    AvailableMoves available_moves;
    Move* past_moves;          // Shouldnt need to worry about the size since we have the upper bound
    uint depth;             // Depth of current node, depth - 1 = index of last move in self.past_moves - Needed for backtracking also
    uint unfinished_white;
    uint unfinished_black;
    char turn;              // WHITE_MOVE / BLACK_MOVE depending who is moving next 
} NodeState;

void NodeDestructor(NodeState* node);
struct Board* load_board(const char* filename);
NodeState* initFirstNode(Board* board);
void GetAvailableMoves(Board* board, NodeState* state);
char isPointInBounds(Point point, Board* board);
char isPointInArea(Point point, Area rea);
uint initialUpperBound(Board* board);
uint getLowerBound(Board* board, NodeState* state);
char getDistanceToClosestPointInArea(Point origin, Area dest);
char getDistanceToFarthestPointInArea(Point origin, Area dest);
char abs_value_trick(char val);
char deleni_dvema_trick(char delenec);
void PrintNode(NodeState* node);
NodeState* CopyNode(Board* board, NodeState* node);
void CopyNodeIntoNode(Board* board, NodeState* source_node, NodeState* target_node);
void NodeMakeMove(Board* board, NodeState* node, Move move);

#endif