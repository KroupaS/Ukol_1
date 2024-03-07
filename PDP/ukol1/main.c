#include "board.h"
#include "solve.h"
#include <time.h>
//#include <unistd.h>


int main(int argc, char** argv) {
    struct timespec start, end;
    double cpu_time;

    // Parse arguments, initialize
    if (argc != 3) {
        printf("Error: program expects two arguments - number of threads to use and filename, Example Usage:\n");
        printf("./vps.out 8 in_0000.txt\n");
        printf("./vps.out 1 in_0002.txt\n");
        return 1;
    }
    const int max_thread = atoi(argv[1]);
    if ((max_thread < 0) || (max_thread > 1000)) {
        printf("Number of threads must be between 1 and 128\n");
    }
    const char* filename = argv[2];
    Board* chessboard = load_board(filename);
    if (chessboard == NULL) {
        printf("Could not initialize board, aborting\n");
    }

    printf("Solving input \"%s\", starting timer\n", filename);

    // Time and solve
    clock_gettime(CLOCK_MONOTONIC, &start);
    NodeState* best_solution = solve(chessboard);
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Seconds in timer struct = %ld\n", end.tv_sec - start.tv_sec);
    printf("Nanoseconds in timer struct = %ld\n", end.tv_nsec - start.tv_nsec);

    cpu_time = (double)((end.tv_sec - start.tv_sec)*1000) + ((double)(end.tv_nsec - start.tv_nsec)) / (double)1000000;

    if (cpu_time > (double)1000) {
        cpu_time /= (double)1000;
        printf("========================================\n");
        printf("| Finished in %.4f seconds |\nBest solution:\n", cpu_time);
    } else {
        // display in ms
        printf("========================================\n");
        printf("| Finished in %.4f ms |\nBest solution:\n", cpu_time);
    }

    if (best_solution->depth == 0) {
        printf("ERROR best solution has depth 0 - correct solution was never found\n");
    } else {
        PrintNode(best_solution);
        NodeDestructor(best_solution);
        free(chessboard);
    }
    
    return 0;
}
