#include "board.h"
#include "solve.h"
#include <time.h>

void PrintMoves(NodeState* node, Board* board);

int main(int argc, char** argv) {
    struct timespec start, end;
    double cpu_time;

    // Parse arguments, initialize
    if (argc != 4) {
        printf("Error: program expects three arguments - number of processes to use filename, and '-v' for verbosity, '-s' for summary Example Usage:\n");
        printf("./vps.out 8 in_0000.txt -s\n");
        printf("./vps.out 1 in_0002.txt -v\n");
        return 1;
    }
    const int max_thread = atoi(argv[1]);
    if ((max_thread < 0) || (max_thread > 1000)) {
        printf("Number of threads must be between 1 and 128, aborting\n");
	return 1;
    }
    const char* filename = argv[2];
    Board* chessboard = load_board(filename);
    if (chessboard == NULL) {
        printf("Could not initialize board, aborting\n");
	return 1;
    }

    if ((strlen(argv[3]) < 2) || ((argv[3][1] != 'v') && (argv[3][1] != 's'))) {
        printf("Could not parse verbosity, use '-v' or '-s' as the last commandline option!\n");
	return 1;
    }
    const char verbosity = argv[3][1];

    printf("Solving input \"%s\", starting timer\n", filename);

    // Time and solve
    clock_gettime(CLOCK_MONOTONIC, &start);
    NodeState* best_solution = solve(chessboard);
    clock_gettime(CLOCK_MONOTONIC, &end);
    //printf("Seconds in timer struct = %ld\n", end.tv_sec - start.tv_sec);
    //printf("Nanoseconds in timer struct = %ld\n", end.tv_nsec - start.tv_nsec);

    cpu_time = (double)((end.tv_sec - start.tv_sec)*1000) + ((double)(end.tv_nsec - start.tv_nsec)) / (double)1000000;

    if (cpu_time > (double)1000) {
        cpu_time /= (double)1000;
	if (verbosity == 'v') {
        	printf("========================================\n");
	}
        printf("| Finished in %.4f seconds |\nBest solution (%u moves):\n", cpu_time, best_solution->depth);
    } else {
        // display in ms
	if (verbosity == 'v') {
        	printf("========================================\n");
	}
        printf("| Finished in %.4f ms |\nBest solution (%u moves):\n", cpu_time, best_solution->depth);
    }

    if (best_solution->depth == 0) {
        printf("ERROR best solution has depth 0 - correct solution was never found\n");
    } else {
        //PrintNode(best_solution);
	if (verbosity == 'v') {
        	PrintMoves(best_solution, chessboard);
	}
        NodeDestructor(best_solution);
        free(chessboard);
    }
    
    return 0;
}


// Functions for visualizing the board and moves of the best solution
//
//

int getFlatIndex(Board* board, unsigned char cell_i, unsigned char cell_j) {
    // [cell_i, cell_j]
    int index = 4 * ((board->m+1) * (cell_j + 1)) + (cell_i + 1) * 4;
    return index;
}

void DrawChar(char* array, Board* board, Point point, char character) {
    int index = getFlatIndex(board, point.X, point.Y);
    array[index + 1] = character;
}

void DrawBorder(char* array, Board* board, Point point) {
    int index = getFlatIndex(board, point.X, point.Y);
    array[index + 3] = '|';
}

void MovePawn(char* array, Board* board, Move move) {
    char pawn = array[getFlatIndex(board, move.Source.X, move.Source.Y) + 1];
    DrawChar(array, board, move.Source, '_');
    DrawChar(array, board, move.Dest, pawn);
}

void PrintBoardSideBySide(char* array_source, char* array_dest, Board* board){

    int middle_row_index = deleni_dvema_trick(board->n);

    for (int i=0; i < board->n+1; i++) {
        for (int j=0; j<board->m+1; j++) {
            printf("%c%c%c%c", array_source[(i*(board->m+1)*4) + j*4], array_source[(i*(board->m+1)*4) + (j*4)+1], array_source[(i*(board->m+1)*4) + (j*4)+2], array_source[(i*(board->m+1)*4) + (j*4)+3]);
        }
        // Here print a line from the second array
        if (i == middle_row_index) {
            printf("   -->  ");
        } else {
            printf("        ");
        }
        for (int j=0; j<board->m+1; j++) {
            printf("%c%c%c%c", array_dest[(i*(board->m+1)*4) + j*4], array_dest[(i*(board->m+1)*4) + (j*4)+1], array_dest[(i*(board->m+1)*4) + (j*4)+2], array_dest[(i*(board->m+1)*4) + (j*4)+3]);
        }
        printf("\n");
    }
}


void PrintMoves(NodeState* node, Board* board) {
    if (board->m > 10) {
        printf("ERROR m is greater than 10, drawing moves aborted");
        return;
    } else if (board->n > 10) {
        printf("ERROR n is greater than 10, drawing moves aborted");
        return;
    }

    // Array for cells of the chessboard - each cell is " B |" or " W |" or " _ |"
    char* board_array = (char*)malloc((board->m + 1) * (board->n + 1) * 4);
    char* board_array_2 = (char*)malloc((board->m + 1) * (board->n + 1) * 4);
    memset(board_array, '_', (board->m + 1) * (board->n + 1) * 4);
    memset(board_array_2, '_', (board->m + 1) * (board->n + 1) * 4);
    //printf("DEBUG size of board_array for m = %u and n = %u => %u\n", board->m, board->n, (board->m + 1) * (board->n + 1) * 4);

    // Initialize header and side
    board_array[0] = ' '; board_array[1] = ' '; board_array[2] = ' '; board_array[3] = ' ';
    board_array_2[0] = ' '; board_array_2[1] = ' '; board_array_2[2] = ' '; board_array_2[3] = ' ';
    for (int i=1; i<board->m+1; i++) {
        board_array[i*4 + 0] = '_';
        board_array[i*4 + 1] = i - 1 + '0';
        board_array[i*4 + 2] = '_';
        board_array[i*4 + 3] = '_';
        board_array_2[i*4 + 0] = '_';
        board_array_2[i*4 + 1] = i - 1 + '0';
        board_array_2[i*4 + 2] = '_';
        board_array_2[i*4 + 3] = '_';
        if (i == board->m) {
            board_array[i*4 + 3] = ' ';
            board_array_2[i*4 + 3] = ' ';
        }
    }
    for (int j=1; j<board->n+1; j++) {
        board_array[j*(board->m+1)*4 + 0] = ' ';
        board_array[j*(board->m+1)*4 + 1] = j-1+'0';
        board_array[j*(board->m+1)*4 + 2] = ' ';
        board_array[j*(board->m+1)*4 + 3] = '|';
        board_array_2[j*(board->m+1)*4 + 0] = ' ';
        board_array_2[j*(board->m+1)*4 + 1] = j-1+'0';
        board_array_2[j*(board->m+1)*4 + 2] = ' ';
        board_array_2[j*(board->m+1)*4 + 3] = '|';
    }

    // Add '|' borders everywhere
    Point point;
    for (int i=0; i<board->m; i++) {
        for (int j=0; j<board->n; j++) {
            point.X = i;
            point.Y = j;
            DrawBorder(board_array, board, point);
            DrawBorder(board_array_2, board, point);
        }
    }


    // Initial pawn positions
    for (int i = board->W_area.top_left.X; i <= board->W_area.bot_right.X; i++) {
        for (int j = board->W_area.top_left.Y; j <= board->W_area.bot_right.Y; j++) {
            point.X = i;
            point.Y = j;
            DrawChar(board_array, board, point, 'W');
            DrawChar(board_array_2, board, point, 'W');
        }
    }
    for (int i = board->B_area.top_left.X; i <= board->B_area.bot_right.X; i++) {
        for (int j = board->B_area.top_left.Y; j <= board->B_area.bot_right.Y; j++) {
            point.X = i;
            point.Y = j;
            DrawChar(board_array, board, point, 'B');
            DrawChar(board_array_2, board, point, 'B');
        }
    }

    //printf("flat index of [2,0] = %i\n", getFlatIndex(board, 2, 0));
    //printf("flat index of [1,1] = %i\n", getFlatIndex(board, 1, 1));
    //printf("flat index of [5,2] = %i\n", getFlatIndex(board, 5, 2));

    // Move pawns and print before and after of each move
    for (unsigned char i=0; i<node->depth; i++){
        printf("Move %u:\n", i+1);
        Move current_move = node->past_moves[i];
        // Make move on board 2 and print
        MovePawn(board_array_2, board, current_move);
        PrintBoardSideBySide(board_array, board_array_2, board);
        MovePawn(board_array, board, current_move);
        printf("\n");
        // Make move also on board 1 before next iteration
        
    }
}
