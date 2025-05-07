#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define COMMAND_FILE "hub_command.txt"

pid_t monitor_pid = -1;
int monitor_running = 0;

void handle_sigchld(int sig) {
    int status;
    waitpid(monitor_pid, &status, 0);
    monitor_pid = -1;
    monitor_running = 0;
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

        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_running) {
                printf("Monitor is already running.\n");
                continue;
            }

            monitor_pid = fork();
            if (monitor_pid == 0) {
                execl("./monitor", "./monitor", NULL);
                perror("execl");
                exit(1);
            } else {
                monitor_running = 1;
                printf("Monitor started (PID %d)\n", monitor_pid);
            }

        } else if (strcmp(input, "stop_monitor") == 0) {
            send_command("stop_monitor");

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

        } else if (strcmp(input, "list_hunts") == 0 ||
                   strncmp(input, "view_treasure", 13) == 0) {
            send_command(input);

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
