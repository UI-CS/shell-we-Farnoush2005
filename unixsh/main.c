#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 64
#define MAX_LINE 1024

int main() {
    char input[MAX_LINE];
    char temp_input[MAX_LINE];
    char *args[MAX_ARGS];
    char *last_command = NULL;  // For command history !!

    while (1) {
        printf("uinxsh> ");
        fflush(stdout);

        if (fgets(input, MAX_LINE, stdin) == NULL) {
            printf("\nGoodbye!\n");
            break;
        }

        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0) {
            continue;
        }

        // Temporary copy to safely check for !!
        strcpy(temp_input, input);

        int i = 0;
        args[i] = strtok(temp_input, " ");
        while (args[i] != NULL && i < MAX_ARGS - 1) {
            i++;
            args[i] = strtok(NULL, " ");
        }
        args[i] = NULL;

        // Check for !! first
        if (args[0] != NULL && strcmp(args[0], "!!") == 0) {
            if (last_command == NULL || strlen(last_command) == 0) {
                printf("No commands in history\n");
                continue;
            }
            printf("%s\n", last_command);
            strcpy(input, last_command);
        } else {
            free(last_command);
            last_command = strdup(input);
        }

        // Tokenize the (possibly updated) input
        int arg_count = 0;
        args[0] = strtok(input, " ");
        while (args[arg_count] != NULL && arg_count < MAX_ARGS - 1) {
            arg_count++;
            args[arg_count] = strtok(NULL, " ");
        }
        args[arg_count] = NULL;

        int pipe_found = 0;
        int pipe_idx = -1;

        // Find pipe position
        for (i = 0; i < arg_count; i++) {
            if (strcmp(args[i], "|") == 0) {
                pipe_idx = i;
                args[i] = NULL;
                pipe_found = 1;
                break;
            }
        }

        char *left_cmd[MAX_ARGS];
        char *right_cmd[MAX_ARGS];

        // Left command (always exists)
        int left_count = pipe_found ? pipe_idx : arg_count;
        for (i = 0; i < left_count; i++) {
            left_cmd[i] = args[i];
        }
        left_cmd[left_count] = NULL;

        // Right command (only if pipe exists)
        if (pipe_found) {
            int right_count = 0;
            for (i = pipe_idx + 1; i < arg_count; i++) {
                right_cmd[right_count++] = args[i];
            }
            right_cmd[right_count] = NULL;
        }

        // Built-in commands (only check left command)
        if (left_cmd[0] != NULL) {
            if (strcmp(left_cmd[0], "exit") == 0) {
                printf("Goodbye!\n");
                free(last_command);
                break;
            }
            if (strcmp(left_cmd[0], "cd") == 0) {
                char *dir = left_cmd[1];
                if (dir == NULL || strcmp(dir, "~") == 0) {
                    dir = getenv("HOME");
                }
                if (chdir(dir) != 0) {
                    printf("cd: directory not found: %s\n", dir ? dir : "home");
                }
                continue;
            }
            if (strcmp(left_cmd[0], "pwd") == 0) {
                char cwd[1024];
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    printf("%s\n", cwd);
                } else {
                    printf("pwd: error getting current directory\n");
                }
                continue;
            }
        }

        // Background execution (&) - only without pipe
        int background = 0;
        if (!pipe_found && arg_count > 0) {
            int last_idx = arg_count - 1;
            if (strcmp(args[last_idx], "&") == 0) {
                background = 1;
                args[last_idx] = NULL;
            }
        }

        // Execute command
        if (pipe_found) {
            // Pipe: cmd1 | cmd2
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe failed");
                continue;
            }

            pid_t pid1 = fork();
            if (pid1 == 0) {  // Child 1: left command
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
                execvp(left_cmd[0], left_cmd);
                printf("Command not found: %s\n", left_cmd[0]);
                exit(1);
            }

            pid_t pid2 = fork();
            if (pid2 == 0) {  // Child 2: right command
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);
                execvp(right_cmd[0], right_cmd);
                printf("Command not found: %s\n", right_cmd[0]);
                exit(1);
            }

            // Parent: close both ends and wait
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
        } else {
            // Normal execution
            pid_t pid = fork();
            if (pid == 0) {  // Child
                execvp(args[0], args);
                printf("Command not found: %s\n", args[0]);
                exit(1);
            } else if (pid > 0) {  // Parent
                if (background) {
                    printf("[Background: %d]\n", pid);
                } else {
                    waitpid(pid, NULL, 0);
                }
            } else {
                printf("Fork error!\n");
            }
        }
    }

    free(last_command);
    return 0;
}