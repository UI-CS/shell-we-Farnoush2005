#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define SIZE 9
#define NUM_THREADS 27  // 9 rows + 9 cols + 9 subgrids

typedef struct {
    int grid[SIZE][SIZE];
    int thread_id;
} thread_arg_t;

int valid[NUM_THREADS] = {0};  // 1 = valid, 0 = invalid
int sudoku_grid[SIZE][SIZE];

// Function to validate row, column, or subgrid
void* validate(void* arg) {
    thread_arg_t* data = (thread_arg_t*)arg;
    int id = data->thread_id;
    int start_row, start_col;

    int seen[SIZE + 1] = {0};

    if (id < 9) {  // Validate rows
        start_row = id;
        start_col = 0;
        for (int col = 0; col < SIZE; col++) {
            int num = sudoku_grid[start_row][col];
            if (num < 1 || num > 9 || seen[num]) {
                valid[id] = 0;
                pthread_exit(NULL);
            }
            seen[num] = 1;
        }
    } else if (id < 18) {  // Validate columns
        start_row = 0;
        start_col = id - 9;
        for (int row = 0; row < SIZE; row++) {
            int num = sudoku_grid[row][start_col];
            if (num < 1 || num > 9 || seen[num]) {
                valid[id] = 0;
                pthread_exit(NULL);
            }
            seen[num] = 1;
        }
    } else {  // Validate 3x3 subgrids
        int subgrid_id = id - 18;
        start_row = (subgrid_id / 3) * 3;
        start_col = (subgrid_id % 3) * 3;
        for (int row = start_row; row < start_row + 3; row++) {
            for (int col = start_col; col < start_col + 3; col++) {
                int num = sudoku_grid[row][col];
                if (num < 1 || num > 9 || seen[num]) {
                    valid[id] = 0;
                    pthread_exit(NULL);
                }
                seen[num] = 1;
            }
        }
    }

    valid[id] = 1;
    pthread_exit(NULL);
}

int main() {
    printf("Please enter the 9x9 Sudoku grid (9 lines, 9 numbers each separated by space):\n");

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (scanf("%d", &sudoku_grid[i][j]) != 1) {
                printf("Invalid input!\n");
                return 1;
            }
        }
    }

    pthread_t threads[NUM_THREADS];
    thread_arg_t args[NUM_THREADS];

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].thread_id = i;
        for (int x = 0; x < SIZE; x++)
            for (int y = 0; y < SIZE; y++)
                args[i].grid[x][y] = sudoku_grid[x][y];

        if (pthread_create(&threads[i], NULL, validate, &args[i]) != 0) {
            perror("Thread creation failed");
            return 1;
        }
    }

    // Wait for all threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Final result
    int is_valid = 1;
    for (int i = 0; i < NUM_THREADS; i++) {
        if (valid[i] == 0) {
            is_valid = 0;
            break;
        }
    }

    if (is_valid) {
        printf("Sudoku is valid! ✅\n");
    } else {
        printf("Sudoku is invalid! ❌\n");
    }

    return 0;
}