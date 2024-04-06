#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

int main(int argc, char** argv) {
    struct timespec start, end;
    double cpu_time;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int num_thr = atoi(argv[1]);

    #pragma omp parallel num_threads(num_thr)
    {
        int thread_id = omp_get_thread_num();
        printf("%d\r", thread_id);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    cpu_time = (double)((end.tv_sec - start.tv_sec)*1000000) + ((double)(end.tv_nsec - start.tv_nsec)) / (double)1000;
    //printf("tv_sec = %ld\n", end.tv_sec - start.tv_sec);
    //printf("tv_nsec = %ld\n", end.tv_nsec - start.tv_nsec);
    if (cpu_time > (double)1000) {
        cpu_time /= (double)1000;
        printf("| %i Threads finished in %.4f ms |\n", num_thr, cpu_time);
    } else {
        // display in ms
        printf("| %i Threads finished in %.4f us |\n", num_thr, cpu_time);
    }
    return 0;
}
