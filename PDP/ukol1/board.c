//#include <stdio.h>
//#include <stdlib.h>
#include "board.h"

int abs_value_trick(int val) {
    // Signed right shift -> result is 32 0's or 32 1's
    int tmp = val >> 31;
    // Flip bits and add one if tmp is 32 1's;
    val ^= tmp;
    val += tmp & 1;
    return val;
}

int deleni_dvema_trick(int delenec) {
    int zaokruhleni = delenec & 1;
    delenec = (delenec >> 1) + zaokruhleni;
    return delenec;
}

int move_cost_compare(const void* first_move, const void* second_move) {
    // Comparison function for stdlib qsort (used to sort available moves by cost)
    MoveAndLowerBound* first = (MoveAndLowerBound*)first_move;
    MoveAndLowerBound* second = (MoveAndLowerBound*)second_move;
    int comparison = first->lower_bound - second->lower_bound;
    return comparison;
}

//void insertion_sort_available_moves(MoveAndLowerBound* move_array, int count) {
    //// This should be faster than quicksort for small arrays theoretically - but practically its so much slower than stdlib's qsort that the program doesnt even finish
    //int i, j;
    //for (i = 1; i < count; i++) {
        //j = i - 1;

        //while ((j >= 0) && (move_array[j].lower_bound > move_array[i].lower_bound)){
            //move_array[j+1] = move_array[j];
            //j = j - 1;
        //}
        //move_array[j+1] = move_array[i];
        //printf("i, j = %i, %i\n", i, j);
    //}

//}


Board* load_board(const char* filename) {
    // Set up the board, done once at start of program. Calculates initial upper bound (which is updated later on as solutions are found)
    FILE* file = fopen(filename, "r"); // Open the file for reading
    if (file == NULL) {
        printf("Error opening file \"%s\".\n", filename);
        return NULL;
    }

    uint data[12];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            if (fscanf(file, "%u", &data[i*4+j]) != 1) { // Read unsigned integer
                printf("Error reading from file.\n");
                fclose(file);
                return NULL;
            }
        }
    }

    Board* board = (Board *)malloc(sizeof(Board));
    board->m = data[0];
    board->n = data[1];
    board->k = data[2];

    board->W_area.top_left.X = data[4];
    board->W_area.top_left.Y = data[5];
    board->W_area.bot_right.X = data[6];
    board->W_area.bot_right.Y = data[7];

    board->B_area.top_left.X = data[8];
    board->B_area.top_left.Y = data[9];
    board->B_area.bot_right.X = data[10];
    board->B_area.bot_right.Y = data[11];

    board->upper_bound = initialUpperBound(board);
    return board;
}

void CopyMoves(NodeState* source_node, NodeState* target_node) {
    target_node->depth = source_node->depth;
    for (int i=0; i < source_node->depth; i++) {
        target_node->past_moves[i] = source_node->past_moves[i];
    }
}

NodeState* initBestSolution(Board* board) {
    NodeState* node_state = (NodeState *)malloc(sizeof(NodeState));
    node_state->White_positions = NULL;
    node_state->Black_positions = NULL;
    node_state->available_moves.Count = 0;
    node_state->available_moves.MovesAndLowerBounds = NULL;
    node_state->past_moves = (Move *)malloc(sizeof(Move) * (board->upper_bound));
    node_state->unfinished_black = 0;
    node_state->unfinished_white = 0;
    node_state->depth = 0;
    node_state->turn = WHITE_MOVE;

    return node_state;
}

NodeState* initFirstNode(Board* board) {
    NodeState* initial_state = (NodeState *)malloc(sizeof(NodeState));
    initial_state->White_positions = (Point *)malloc(sizeof(Point) * board->k);
    initial_state->Black_positions = (Point *)malloc(sizeof(Point) * board->k);
    initial_state->available_moves.Count = 0;
    initial_state->available_moves.MovesAndLowerBounds = NULL;
    initial_state->past_moves = (Move *)malloc(sizeof(Move) * board->upper_bound);
    initial_state->unfinished_black = board->k;
    initial_state->unfinished_white = board->k;
    initial_state->depth = 0;
    initial_state->turn = WHITE_MOVE;


    // Initialize white positions
    Point bod;
    uint pawn_index = 0;
    for (int i = board->W_area.top_left.X; i <= board->W_area.bot_right.X; i++) {
        for (int j = board->W_area.top_left.Y; j <= board->W_area.bot_right.Y; j++) {
            bod.X = i;
            bod.Y = j;
            initial_state->White_positions[pawn_index] = bod;
            pawn_index++;
        }
    }
    // Initialize Black positions
    pawn_index = 0;
    for (int i = board->B_area.top_left.X; i <= board->B_area.bot_right.X; i++) {
        for (int j = board->B_area.top_left.Y; j <= board->B_area.bot_right.Y; j++) {
            bod.X = i;
            bod.Y = j;
            initial_state->Black_positions[pawn_index] = bod;
            pawn_index++;
        }
    }

    return initial_state;
}

void PrintNode(NodeState* node) {
    printf("Node depth = %u\n", node->depth);
    printf("Unfinished white = %u\n", node->unfinished_white);
    printf("Unfinished black = %u\n", node->unfinished_black);
    printf("Next to turn = %u\n", node->turn);
    printf("Past moves: \n");
    for (uint i=0; i<node->depth; i++) {
        printf("[%i, %i] -> [%i, %i]\n", node->past_moves[i].Source.X, node->past_moves[i].Source.Y, node->past_moves[i].Dest.X, node->past_moves[i].Dest.Y);
    }
    if (node->available_moves.Count > 0) {
        printf("%u available moves and costs: \n", node->available_moves.Count);
        for (int i=0; i<node->available_moves.Count; i++) {
            printf("[%i, %i] -> [%i, %i], Cost = %u\n", node->available_moves.MovesAndLowerBounds[i].move.Source.X, node->available_moves.MovesAndLowerBounds[i].move.Source.Y, node->available_moves.MovesAndLowerBounds[i].move.Dest.X, node->available_moves.MovesAndLowerBounds[i].move.Dest.Y, node->available_moves.MovesAndLowerBounds[i].lower_bound);
        }
    } else {
        printf("Available move count = 0\n");
    }

}

NodeState* CopyNode(Board* board, NodeState* node) {
    // Copies all members of <node> to new struct except <node.available_moves>
    // Used for creation of new states
    NodeState* new_node = (NodeState*)malloc(sizeof(NodeState));
    new_node->White_positions = (Point *)malloc(sizeof(Point) * board->k);
    new_node->Black_positions = (Point *)malloc(sizeof(Point) * board->k);
    for(int i=0; i<board->k; i++){
        new_node->White_positions[i] = node->White_positions[i];
        new_node->Black_positions[i] = node->Black_positions[i];
    }
    new_node->available_moves.Count = 0;
    new_node->available_moves.MovesAndLowerBounds = NULL;
    new_node->past_moves = (Move *)malloc(sizeof(Move) * board->upper_bound);
    for(uint i=0; i<node->depth; i++) {
        new_node->past_moves[i] = node->past_moves[i];
    }
    new_node->unfinished_black = node->unfinished_black;
    new_node->unfinished_white = node->unfinished_white;
    new_node->depth = node->depth;
    new_node->turn = node->turn;

    return new_node;
}

//void CopyNodeIntoNode(Board* board, NodeState* source_node, NodeState* target_node) {
    //// Deep copies all members of <node> to <target_node> except available moves
    //// Used only to save current best solutions
    //target_node->White_positions = (Point *)malloc(sizeof(Point) * board->k);
    //target_node->Black_positions = (Point *)malloc(sizeof(Point) * board->k);
    //for(int i=0; i<board->k; i++){
        //target_node->White_positions[i] = source_node->White_positions[i];
        //target_node->Black_positions[i] = source_node->Black_positions[i];
    //}
    //target_node->available_moves.Count = 0;
    //target_node->available_moves.MovesAndLowerBounds = NULL;
    //target_node->past_moves = (Move *)malloc(sizeof(Move) * board->upper_bound);
    //for(uint i=0; i<source_node->depth; i++) {
        //target_node->past_moves[i] = source_node->past_moves[i];
    //}
    //target_node->unfinished_black = source_node->unfinished_black;
    //target_node->unfinished_white = source_node->unfinished_white;
    //target_node->depth = source_node->depth;
    //target_node->turn = source_node->turn;
//}

void NodeMakeMove(Board* board, NodeState* node, Move move) {
    // Increment depth, check if move ends in finish area -> decrement unfinished, change White/Black positions, flip turn, Add to past moves
    
    // Make move
    // Ineffective - have to find pawn to make move with first, but its still slightly faster than when we carry around an additional index in the Move struct.
    //      -> TODO Best solution would be to replace move.Source with the index, we should be able to do without that
    if (node->turn == WHITE_MOVE) {
        for (uint i = 0; i<board->k; i++){
            if ((move.Source.X == node->White_positions[i].X) && (move.Source.Y == node->White_positions[i].Y)) {
                node->White_positions[i].X = move.Dest.X;
                node->White_positions[i].Y = move.Dest.Y;
                if ((getDistanceToClosestPointInArea(move.Dest, board->B_area) == 0) && (getDistanceToClosestPointInArea(move.Source, board->B_area) > 0)) {
                    // If moving into finish zone, decrement unfinished_white
                    node->unfinished_white = node->unfinished_white - 1;
                } else if ((getDistanceToClosestPointInArea(move.Dest, board->B_area) > 0) && (getDistanceToClosestPointInArea(move.Source, board->B_area) == 0)) {
                    // If moving out of finish zone, increment unfinished_white
                    node->unfinished_white = node->unfinished_white + 1;
                }
            }
        }
        if (node->unfinished_black > 0) {
            node->turn = BLACK_MOVE;
        }
    } else {
        for (uint i = 0; i<board->k; i++){
            if ((move.Source.X == node->Black_positions[i].X) && (move.Source.Y == node->Black_positions[i].Y)) {
                node->Black_positions[i].X = move.Dest.X;
                node->Black_positions[i].Y = move.Dest.Y;
                if ((getDistanceToClosestPointInArea(move.Dest, board->W_area) == 0) && (getDistanceToClosestPointInArea(move.Source, board->W_area) > 0)) {
                    node->unfinished_black = node->unfinished_black - 1;
                } else if ((getDistanceToClosestPointInArea(move.Dest, board->W_area) > 0) && (getDistanceToClosestPointInArea(move.Source, board->W_area) == 0)) {
                    node->unfinished_black = node->unfinished_black + 1;
                }
            }
        }
        if (node->unfinished_white > 0) {
            node->turn = WHITE_MOVE;
        }
    }

    node->depth += 1;
    node->past_moves[(node->depth)-1] = move;
}

void NodeDestructor(NodeState* node) {
    // Cant call if GetAvailableMoves was not called on this node - would free nullptr
    free(node->White_positions);
    free(node->Black_positions);
    if(node->available_moves.MovesAndLowerBounds != NULL) {
        free(node->available_moves.MovesAndLowerBounds);
    }
    free(node->past_moves);
    free(node);
}

void FreeNodeMembers(NodeState* node) {
    // Cant call if GetAvailableMoves was not called on this node - would free nullptr
    free(node->White_positions);
    free(node->Black_positions);
    if(node->available_moves.MovesAndLowerBounds != NULL) {
        free(node->available_moves.MovesAndLowerBounds);
    }
    free(node->past_moves);
}

int isPointInBounds(Point point, Board* board) {
    if ((point.X < 0) || (point.Y < 0)) {
        return 0;
    }
    if ((point.X > (board->m-1)) || (point.Y > (board->n-1))) {
        return 0;
    }
    return 1;
}

int isPointInArea(Point point, Area area) {
    if ((point.X < area.top_left.X) || (point.X > area.bot_right.X) || (point.Y < area.top_left.Y) || (point.Y > area.bot_right.Y)){
        return 0;
    }
    return 1;
}

int isPawnFinished(Board* board, NodeState* state, Point pawn_position) {
    // Unused
    // return 1 if white pawn is in B or black pawn in W
    if (state->turn == WHITE_MOVE) {
        // Its white to move, check if this pawn is in B
        return isPointInArea(pawn_position, board->B_area);
    } else {
        return isPointInArea(pawn_position, board->W_area);
    }
}


int isDestinationValid(Point point, Board* board, NodeState* state) {
    // Look through all state positions if there is already a pawn at <point> or if its out of bounds
    if (isPointInBounds(point, board)) {
        // Check taken positions
        for (uint i = 0; i<board->k; i++) {
            if ((state->White_positions[i].X == point.X) && (state->White_positions[i].Y == point.Y)){
                //printf("Destination invalid [%i, %i], bottom right = [%i, %i]\n", point.X, point.Y, board->m-1, board->n-1);
                return 0;
            }
            if ((state->Black_positions[i].X == point.X) && (state->Black_positions[i].Y == point.Y)){
                return 0;
            }
        }
    } else {
        return 0;
    }
    return 1;
}

uint initialUpperBound(Board* board) {
    uint upper_bound = 0;
    Point bod;
    int distance;
    // First calculate for white pawns
    for (int i = board->W_area.top_left.X; i <= board->W_area.bot_right.X; i++) {
        for (int j = board->W_area.top_left.Y; j <= board->W_area.bot_right.Y; j++) {
            //printf("bod = [%i, %i]\n", i, j);
            bod.X = i;
            bod.Y = j;
            distance = getDistanceToFarthestPointInArea(bod, board->B_area);
            //printf("farthest point distance = %i\n", distance);
            upper_bound += distance;
        }
    }
    for (int i = board->B_area.top_left.X; i <= board->B_area.bot_right.X; i++) {
        for (int j = board->B_area.top_left.Y; j <= board->B_area.bot_right.Y; j++) {
            //printf("bod = [%i, %i]\n", i, j);
            bod.X = i;
            bod.Y = j;
            distance = getDistanceToFarthestPointInArea(bod, board->W_area);
            //printf("farthest point distance = %i\n", distance);
            upper_bound += distance;
        }
    }
    return upper_bound;
}

uint getLowerBound(Board* board, NodeState* state) {
    uint lower_bound = 0;
    int distance;

    // First calculate for white pawns
    for (int i=0; i<board->k; i++) {
        distance = getDistanceToClosestPointInArea(state->White_positions[i], board->B_area);
        lower_bound += distance;
    }
    for (int i=0; i<board->k; i++) {
        distance = getDistanceToClosestPointInArea(state->Black_positions[i], board->W_area);
        lower_bound += distance;
    }

    return lower_bound;
}

int ManhattanDist(Point first, Point second) {
    int dist = abs_value_trick(first.X - second.X);
    dist += abs_value_trick(first.Y - second.Y);
    return dist;
}

int getDistanceToClosestPointInArea(Point origin, Area dest_area) {
    // First calculate distance in x;
    int dist = 0;
    //printf("Point = [%i, %i]\n", origin.X, origin.Y);
    int dist_1 = abs_value_trick(dest_area.top_left.X - origin.X);
    int dist_2 = abs_value_trick(dest_area.bot_right.X - origin.X);
    // Special case where point's X is between the corners' X
    if (((origin.X > dest_area.top_left.X) && (dest_area.bot_right.X > origin.X)) || ((origin.X > dest_area.bot_right.X) && (dest_area.top_left.X > origin.X))) {
        dist = 0;
    } else {
        if (dist_1 < dist_2) {
            dist = deleni_dvema_trick(dist_1);
            //printf("X dist = %i\n", dist);
        } else {
            dist = deleni_dvema_trick(dist_2);
            //printf("X dist = %i\n", dist);
        }
    }
    //printf("X dist 1 = %i, X dist 2 = %i\n", dist_1, dist_2);
    
    dist_1 = abs_value_trick(dest_area.top_left.Y - origin.Y);
    dist_2 = abs_value_trick(dest_area.bot_right.Y - origin.Y);
    if (((origin.Y > dest_area.top_left.Y) && (dest_area.bot_right.Y > origin.Y)) || ((origin.Y > dest_area.bot_right.Y) && (dest_area.top_left.Y > origin.Y))) {
        dist += 0;
    } else {
        if (dist_1 < dist_2) {
            dist += deleni_dvema_trick(dist_1);
            //printf("Y dist = %i\n", dist);
        } else {
            dist += deleni_dvema_trick(dist_2);
            //printf("Y dist = %i\n", dist);
        }
    }
    //printf("Y dist 1 = %i, Y dist 2 = %i\n", dist_1, dist_2);
    //printf("Total dist = %i\n", dist);
    return dist;
}


int getDistanceToFarthestPointInArea(Point origin, Area dest) {
    // Pick one corner arbitrarily
    Point farthest = dest.top_left;
    int distance = ManhattanDist(origin, farthest);

    // Get distance to the other corners and compare
    Point candidate = dest.bot_right;
    int distance_candidate = ManhattanDist(origin, candidate);
    // Unnecessary conditional 
    if (distance_candidate > distance) { farthest = candidate; distance = distance_candidate; }

    candidate.X = dest.top_left.X;
    candidate.Y = dest.bot_right.Y;
    distance_candidate = ManhattanDist(origin, candidate);
    if (distance_candidate > distance) { farthest = candidate; distance = distance_candidate; }

    candidate.X = dest.bot_right.X;
    candidate.Y = dest.top_left.Y;
    distance_candidate = ManhattanDist(origin, candidate);
    if (distance_candidate > distance) { farthest = candidate; distance = distance_candidate; }

    return distance;
}

uint getCandidateLowerBound(uint pawn_index, Point dest, NodeState* node, Board* board) {
    // Makes a temporary move in the current state - calculates lower bound - removes temporary move
    // Useful to calculate lower bounds of candidates when looking for Available Moves without copying a node
    if (node->turn == WHITE_MOVE) {
        Point pawn_backup = node->White_positions[pawn_index];
        node->White_positions[pawn_index].X = dest.X;
        node->White_positions[pawn_index].Y = dest.Y;
        uint lower_bound = getLowerBound(board, node);
        node->White_positions[pawn_index].X = pawn_backup.X;
        node->White_positions[pawn_index].Y = pawn_backup.Y;
        return lower_bound;
    } else {
        Point pawn_backup = node->Black_positions[pawn_index];
        node->Black_positions[pawn_index].X = dest.X;
        node->Black_positions[pawn_index].Y = dest.Y;
        uint lower_bound = getLowerBound(board, node);
        node->Black_positions[pawn_index].X = pawn_backup.X;
        node->Black_positions[pawn_index].Y = pawn_backup.Y;
        return lower_bound;
    }

}


void GetAvailableMoves(Board* board, NodeState* state) {
    //int current_cost; // For calculating costs of each move 
    int candidate_cost;
    Point candidate_dest;
    MoveAndLowerBound valid_move_and_cost;
    
    //state->available_moves.Count = 0;
    state->available_moves.MovesAndLowerBounds = (MoveAndLowerBound*)malloc(sizeof(MoveAndLowerBound) * board->k * 4); // Each pawn can have at most 4 moves - one move in every direction (Hopping over a pawn means a normal move is not possible). 

    //if ((state->unfinished_black == 0) && (state->unfinished_white == 0)) {
        //// TODO this is useless - just an assertion. Impossible for both sides to have no moves.
        //printf("ERROR Finished state no turns\n");
        //PrintNode(state);
        //return;
    //}

    if (state->turn == WHITE_MOVE) {
        for (uint i = 0; i<board->k; i++) {
            // Generate moves for White_positions[i], each one that is valid check its cost change and add it to the list
            // Check moves in X direction
            candidate_dest.X = state->White_positions[i].X + 1;
            candidate_dest.Y = state->White_positions[i].Y;
            if(isDestinationValid(candidate_dest, board, state)) {
                // TODO calculate lower bound after this move, not cost of the move
                candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                valid_move_and_cost.lower_bound = candidate_cost;
                valid_move_and_cost.move.Source = state->White_positions[i];
                valid_move_and_cost.move.Dest = candidate_dest;
                state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                state->available_moves.Count++;
            } else {
                // Regular move is not available, try double move 
                candidate_dest.X = state->White_positions[i].X + 2;
                candidate_dest.Y = state->White_positions[i].Y;
                if(isDestinationValid(candidate_dest, board, state)) {
                    candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                    valid_move_and_cost.lower_bound = candidate_cost;
                    valid_move_and_cost.move.Source = state->White_positions[i];
                    valid_move_and_cost.move.Dest = candidate_dest;
                    state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                    state->available_moves.Count++;
                }
            }
            candidate_dest.X = state->White_positions[i].X - 1;
            candidate_dest.Y = state->White_positions[i].Y;
            if(isDestinationValid(candidate_dest, board, state)) {
                candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                valid_move_and_cost.lower_bound = candidate_cost; 
                valid_move_and_cost.move.Source = state->White_positions[i];
                valid_move_and_cost.move.Dest = candidate_dest;
                state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                state->available_moves.Count++;
            } else {
                candidate_dest.X = state->White_positions[i].X - 2;
                candidate_dest.Y = state->White_positions[i].Y;
                if(isDestinationValid(candidate_dest, board, state)) {
                    candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                    valid_move_and_cost.lower_bound = candidate_cost;
                    valid_move_and_cost.move.Source = state->White_positions[i];
                    valid_move_and_cost.move.Dest = candidate_dest;
                    state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                    state->available_moves.Count++;
                }
            }

            // Check moves in Y direction
            candidate_dest.X = state->White_positions[i].X;
            candidate_dest.Y = state->White_positions[i].Y + 1;
            if(isDestinationValid(candidate_dest, board, state)) {
                candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                valid_move_and_cost.lower_bound = candidate_cost;
                valid_move_and_cost.move.Source = state->White_positions[i];
                valid_move_and_cost.move.Dest = candidate_dest;
                state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                state->available_moves.Count++;
            } else {
                candidate_dest.X = state->White_positions[i].X;
                candidate_dest.Y = state->White_positions[i].Y + 2;
                if(isDestinationValid(candidate_dest, board, state)) {
                    candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                    valid_move_and_cost.lower_bound = candidate_cost;
                    valid_move_and_cost.move.Source = state->White_positions[i];
                    valid_move_and_cost.move.Dest = candidate_dest;
                    state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                    state->available_moves.Count++;
                }
            }
            candidate_dest.X = state->White_positions[i].X;
            candidate_dest.Y = state->White_positions[i].Y - 1;
            if(isDestinationValid(candidate_dest, board, state)) {
                candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                valid_move_and_cost.lower_bound = candidate_cost;
                valid_move_and_cost.move.Source = state->White_positions[i];
                valid_move_and_cost.move.Dest = candidate_dest;
                state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                state->available_moves.Count++;
            } else {
                candidate_dest.X = state->White_positions[i].X;
                candidate_dest.Y = state->White_positions[i].Y - 2;
                if(isDestinationValid(candidate_dest, board, state)) {
                    candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                    valid_move_and_cost.lower_bound = candidate_cost;
                    valid_move_and_cost.move.Source = state->White_positions[i];
                    valid_move_and_cost.move.Dest = candidate_dest;
                    state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                    state->available_moves.Count++;
                }
            }
        }
        qsort(state->available_moves.MovesAndLowerBounds, state->available_moves.Count, sizeof(MoveAndLowerBound), move_cost_compare);
        return;
    } else {
        // Its blacks turn, because either 1) white is finished and black isnt, turn doesnt matter or 2) its BLACK_MOVE
        for (uint i = 0; i<board->k; i++) {
            // Generate moves for Black_positions[i], each one that is valid check its cost change and add it to the list
            // Check moves in X direction
            candidate_dest.X = state->Black_positions[i].X + 1;
            candidate_dest.Y = state->Black_positions[i].Y;
            if(isDestinationValid(candidate_dest, board, state)) {
                candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                valid_move_and_cost.lower_bound = candidate_cost;
                valid_move_and_cost.move.Source = state->Black_positions[i];
                valid_move_and_cost.move.Dest = candidate_dest;
                state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                state->available_moves.Count++;
            } else {
                candidate_dest.X = state->Black_positions[i].X + 2;
                candidate_dest.Y = state->Black_positions[i].Y;
                if(isDestinationValid(candidate_dest, board, state)) {
                    candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                    valid_move_and_cost.lower_bound = candidate_cost;
                    valid_move_and_cost.move.Source = state->Black_positions[i];
                    valid_move_and_cost.move.Dest = candidate_dest;
                    state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                    state->available_moves.Count++;
                }
            }
            candidate_dest.X = state->Black_positions[i].X - 1;
            candidate_dest.Y = state->Black_positions[i].Y;
            if(isDestinationValid(candidate_dest, board, state)) {
                candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                valid_move_and_cost.lower_bound = candidate_cost;
                valid_move_and_cost.move.Source = state->Black_positions[i];
                valid_move_and_cost.move.Dest = candidate_dest;
                state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                state->available_moves.Count++;
            } else {
                candidate_dest.X = state->Black_positions[i].X - 2;
                candidate_dest.Y = state->Black_positions[i].Y;
                if(isDestinationValid(candidate_dest, board, state)) {
                    candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                    valid_move_and_cost.lower_bound = candidate_cost;
                    valid_move_and_cost.move.Source = state->Black_positions[i];
                    valid_move_and_cost.move.Dest = candidate_dest;
                    state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                    state->available_moves.Count++;
                }
            }

            // Check moves in Y direction
            candidate_dest.X = state->Black_positions[i].X;
            candidate_dest.Y = state->Black_positions[i].Y + 1;
            if(isDestinationValid(candidate_dest, board, state)) {
                candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                valid_move_and_cost.lower_bound = candidate_cost;
                valid_move_and_cost.move.Source = state->Black_positions[i];
                valid_move_and_cost.move.Dest = candidate_dest;
                state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                state->available_moves.Count++;
            } else {
                candidate_dest.X = state->Black_positions[i].X;
                candidate_dest.Y = state->Black_positions[i].Y + 2;
                if(isDestinationValid(candidate_dest, board, state)) {
                    candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                    valid_move_and_cost.lower_bound = candidate_cost;
                    valid_move_and_cost.move.Source = state->Black_positions[i];
                    valid_move_and_cost.move.Dest = candidate_dest;
                    state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                    state->available_moves.Count++;
                }
            }
            candidate_dest.X = state->Black_positions[i].X;
            candidate_dest.Y = state->Black_positions[i].Y - 1;
            if(isDestinationValid(candidate_dest, board, state)) {
                candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                valid_move_and_cost.lower_bound = candidate_cost;
                valid_move_and_cost.move.Source = state->Black_positions[i];
                valid_move_and_cost.move.Dest = candidate_dest;
                state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                state->available_moves.Count++;
            } else {
                candidate_dest.X = state->Black_positions[i].X;
                candidate_dest.Y = state->Black_positions[i].Y - 2;
                if(isDestinationValid(candidate_dest, board, state)) {
                    candidate_cost = getCandidateLowerBound(i, candidate_dest, state, board);
                    valid_move_and_cost.lower_bound = candidate_cost;
                    valid_move_and_cost.move.Source = state->Black_positions[i];
                    valid_move_and_cost.move.Dest = candidate_dest;
                    state->available_moves.MovesAndLowerBounds[state->available_moves.Count] = valid_move_and_cost;
                    state->available_moves.Count++;
                }
            }
        }
        qsort(state->available_moves.MovesAndLowerBounds, state->available_moves.Count, sizeof(MoveAndLowerBound), move_cost_compare);
        return;
    }
}
