#ifndef BOARD_H
#define BOARD_H

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <stddef.h>

typedef unsigned int uint;

#define WHITE_MOVE 0
#define BLACK_MOVE 1

// [X,Y] -> X is horizontal axis, Y is vertical axis. Origin is top left
typedef struct Point {      // 2 bytes
    unsigned char X;
    unsigned char Y;
} Point;

typedef struct Move {       // 4 bytes
    Point Source;           
    Point Dest;
} Move;

typedef struct MoveAndLowerBound {      // 8 bytes
    Move move;                          
    uint lower_bound;  // Should be from -2 to +2 including
} MoveAndLowerBound;

// Used to store possible moves from a position, and moves that were already made
typedef struct AvailableMoves {         // 4 bytes + pointer
    uint Count;                             // Number of possible moves should be <= (400/2) * 8
    MoveAndLowerBound* MovesAndLowerBounds;
} AvailableMoves;

typedef struct AvailableMovesSerial {
    uint Count;                                                                             // 4
    MoveAndLowerBound MovesAndLowerBoundsArray[sizeof(MoveAndLowerBound) * 8 * 4];          // 260
} AvailableMovesSerial;

typedef struct Area {    
    Point top_left;  
    Point bot_right; // 4 bytes
} Area;

// This struct will be initialized once, all nodes have a pointer to it
typedef struct Board {
    Area W_area;                 // 4 
    Area B_area;                 // 8
    uint upper_bound;            // 12
    uint lower_bound;            // 16
    unsigned char k;             // 20 Maximum k should be <= 400/2
    unsigned char m;             // 21
    unsigned char n;             // 22
    unsigned char padding[2];    // 24   
} Board;

typedef struct NodeState {
    Point* White_positions;             // 8 bytes
    Point* Black_positions;             // 16 bytes - Board.k size arrays
    AvailableMoves available_moves;     // 28 bytes
    Move* past_moves;                   // 36 bytes
    unsigned char depth;                // - Depth of current node, depth - 1 = index of last move in self.past_moves - Needed for backtracking also
    unsigned char unfinished_white;     // 
    unsigned char unfinished_black;     // 
    unsigned char turn;                 // 40 - WHITE_MOVE / BLACK_MOVE depending who is moving next 
} NodeState;

// Serialized NodeState - array sizes are just as big as needed for the known input data
typedef struct NodeStateSerial {
    Board board;                            // 24
    Point White_positions[8];               // 32
    Point Black_positions[8];               // 40
    Move past_moves[48];                    // 88
    unsigned char depth;                    //
    unsigned char unfinished_white;         //
    unsigned char unfinished_black;         //
    unsigned char turn;                     // 92
} NodeStateSerial;

struct Board* load_board(const char* filename);
NodeState* initBestSolution(Board* board);
int isPointInBounds(Point point, Board* board);
int isPointInArea(Point point, Area rea);
uint initialUpperBound(Board* board);
uint getLowerBound(Board* board, NodeState* state);
int getDistanceToClosestPointInArea(Point origin, Area dest);
int getDistanceToFarthestPointInArea(Point origin, Area dest);
int abs_value_trick(int val);
int deleni_dvema_trick(int delenec);

NodeState* initFirstNode(Board* board);
void PrintNode(NodeState* node);
NodeState* CopyNode(Board* board, NodeState* node);
void CopyNodeIntoNode(Board* board, NodeState* source_node, NodeState* target_node);
void NodeMakeMove(Board* board, NodeState* node, Move move);
void NodeDestructor(NodeState* node);
void FreeBestSolution(NodeState* node);

void GetAvailableMoves(Board* board, NodeState* state);
void CopyMoves(NodeState* source_node, NodeState* target_node);

void UpdateSolutionFromSerial(NodeStateSerial* serialized_state, NodeState* best_solution);

void SerializeNodeState(NodeStateSerial* serialized_state, Board* board, NodeState* node);
void SerializeSolution(NodeStateSerial* serialized_state, NodeState* node);
NodeState* DeSerializeNodeState(Board* board, NodeStateSerial* serialized_state);
NodeState* DeSerializeSolution(NodeStateSerial* serialized_state);

#endif