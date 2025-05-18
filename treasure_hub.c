#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>

#define COMMAND_FILE "hub_command.txt"

pid_t monitor_pid = -1;
int monitor_running = 0;
int monitor_pipe_fd = -1;

void handle_sigchld(int sig) {
    int status;
    waitpid(monitor_pid, &status, 0);
    monitor_pid = -1;
    monitor_running = 0;
    close(monitor_pipe_fd);
    monitor_pipe_fd = -1;
    printf("Monitor terminated with status %d\n", status);
}

void send_command(const char *cmd) {
    if (!monitor_running) {
        printf("Error: Monitor is not running.\n");
        return;
    }

    FILE *f = fopen(COMMAND_FILE, "w");
    if (!f) {
        perror("fopen");
        return;
    }
    fprintf(f, "%s\n", cmd);
    fclose(f);

    kill(monitor_pid, SIGUSR1);

    usleep(100000); 
    if (monitor_pipe_fd != -1) {
        char buf[256];
        int n;
        while ((n = read(monitor_pipe_fd, buf, sizeof(buf) - 1)) > 0) {
            buf[n] = '\0';
            printf("%s", buf);
            if (n < 255) break;
        }
    }
}

void handle_calculate_score() {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strncmp(entry->d_name, "hunt:", 5) == 0) {
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                execl("./calculate_score", "./calculate_score", entry->d_name, NULL);
                perror("execl failed");
                exit(1);
            } else {
                close(pipefd[1]);
                printf("Hunt: %s\n", entry->d_name);
                char buf[256];
                int n;
                while ((n = read(pipefd[0], buf, sizeof(buf)-1)) > 0) {
                    buf[n] = '\0';
                    printf("%s", buf);
                }
                close(pipefd[0]);
                waitpid(pid, NULL, 0);
            }
        }
    }
    closedir(dir);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGCHLD, &sa, NULL);

    char input[128];

    while (1) {
        printf("hub> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin))
            break;

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_running) {
                printf("Monitor is already running.\n");
                continue;
            }

            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                continue;
            }

            monitor_pid = fork();
            if (monitor_pid == 0) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                execl("./monitor", "./monitor", NULL);
                perror("execl");
                exit(1);
            } else {
                close(pipefd[1]);
                monitor_pipe_fd = pipefd[0];
                monitor_running = 1;
                printf("Monitor started (PID %d)\n", monitor_pid);
            }

        } else if (strcmp(input, "stop_monitor") == 0) {
            send_command("stop_monitor");

        } else if (strcmp(input, "list_hunts") == 0) {
            send_command("list_hunts");

        } else if (strcmp(input, "list_treasures") == 0) {
            char hunt_id[64];
            printf("Enter hunt ID: ");
            fflush(stdout);
            if (fgets(hunt_id, sizeof(hunt_id), stdin)) {
                hunt_id[strcspn(hunt_id, "\n")] = 0;
                char command[128];
                snprintf(command, sizeof(command), "list_treasures %s", hunt_id);
                send_command(command);
            }

        } else if (strcmp(input, "view_treasure") == 0) {
            char hunt_id[64], treasure_id_str[16];
            int treasure_id;

            printf("Enter hunt ID: ");
            if (!fgets(hunt_id, sizeof(hunt_id), stdin)) continue;
            hunt_id[strcspn(hunt_id, "\n")] = 0;

            printf("Enter treasure ID: ");
            if (!fgets(treasure_id_str, sizeof(treasure_id_str), stdin)) continue;
            treasure_id_str[strcspn(treasure_id_str, "\n")] = 0;
            treasure_id = atoi(treasure_id_str);

            char command[128];
            snprintf(command, sizeof(command), "view_treasure %s %d", hunt_id, treasure_id);
            send_command(command);

        } else if (strcmp(input, "calculate_score") == 0) {
            handle_calculate_score();

        } else if (strcmp(input, "exit") == 0) {
            if (monitor_running) {
                printf("Error: Cannot exit while monitor is running.\n");
            } else {
                break;
            }

        } else {
            printf("Unknown command.\n");
        }
    }

    return 0;
}

