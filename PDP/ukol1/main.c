#include "board.h"
#include "solve.h"


int main(int argc, char** argv) {

    //const char* filenames[] = { "in_0000.txt", "in_0001.txt", "in_0002.txt", "in_0009.txt", "in_0011.txt", "in_0015.txt", "in_0016.txt", "in_0017.txt" };

    if (argc != 2) {
        printf("Error: program expects one argument, Example Usage:\n");
        printf("./vps.out in_0000.txt\n");
        return 1;
    }

    const char* filename = argv[1];
    // in_0017 a in_0015 jsou o jedna vic nez referencni
    size_t recursion_counter = 0;
    size_t* p_counter = &recursion_counter;
    Board* chessboard = load_board(filename);
    printf("Solving input \"%s\", starting timer\n", filename);
    // TODO timer
    NodeState* best_solution = solve(chessboard, p_counter);
    // TODO end timer, output best solution
    printf("Solve recurse finished with %zu recursive calls, best solution:\n", recursion_counter);
    PrintNode(best_solution);
    NodeDestructor(best_solution);
    free(chessboard);

    
    return 0;
}