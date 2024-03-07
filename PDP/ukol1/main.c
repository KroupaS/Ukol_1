#include "board.h"
#include "solve.h"

#include <time.h>


int main(int argc, char** argv) {
    clock_t start, end;
    double cpu_time;
    size_t recursion_counter;
    size_t* p_counter;

    if (argc != 2) {
        printf("Error: program expects one argument, Example Usage:\n");
        printf("./vps.out in_0000.txt\n");
        return 1;
    }

    const char* filename = argv[1];
    recursion_counter = 0;
    p_counter = &recursion_counter;
    Board* chessboard = load_board(filename);
    printf("Solving input \"%s\", starting timer\n", filename);
    start = clock();
    NodeState* best_solution = solve(chessboard, p_counter);
    end = clock();
    cpu_time = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Solve recurse finished with %zu recursive calls in %.4f seconds, best solution:\n", recursion_counter, cpu_time);
    PrintNode(best_solution);
    NodeDestructor(best_solution);
    free(chessboard);
    
    return 0;
}