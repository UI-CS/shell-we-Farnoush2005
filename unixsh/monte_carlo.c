#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <num_processes> <total_points>\n", argv[0]);
        printf("Example: %s 4 100000000\n", argv[0]);
        return 1;
    }

    int num_processes = atoi(argv[1]);
    long long total_points = atoll(argv[2]);
    long long points_per_process = total_points / num_processes;
    long long remaining = total_points % num_processes;

    int pipes[num_processes][2];
    pid_t pids[num_processes];

    // Create pipes
    for (int i = 0; i < num_processes; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe failed");
            return 1;
        }
    }

    // Create child processes
    for (int i = 0; i < num_processes; i++) {
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork failed");
            return 1;
        }

        if (pids[i] == 0) {  // Child process
            long long points = points_per_process + (i < remaining ? 1 : 0);
            long long inside = 0;
            unsigned int seed = time(NULL) ^ (getpid() << 16);

            for (long long j = 0; j < points; j++) {
                double x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
                double y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
                if (x * x + y * y <= 1.0) {
                    inside++;
                }
            }

            // Send result to parent
            close(pipes[i][0]);
            write(pipes[i][1], &inside, sizeof(inside));
            close(pipes[i][1]);
            exit(0);
        }
    }

    // Parent: collect results
    long long total_inside = 0;
    for (int i = 0; i < num_processes; i++) {
        long long inside;
        close(pipes[i][1]);
        read(pipes[i][0], &inside, sizeof(inside));
        total_inside += inside;
        close(pipes[i][0]);
        wait(NULL);
    }

    double pi_estimate = 4.0 * total_inside / total_points;
    printf("Total points: %lld\n", total_points);
    printf("Points inside circle: %lld\n", total_inside);
    printf("Estimated π = %.10f\n", pi_estimate);
    printf("Error = %.10f\n", pi_estimate - 3.1415926535);

    return 0;
}