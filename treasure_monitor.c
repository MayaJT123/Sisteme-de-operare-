#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h> 

#define COMMAND_FILE "hub_command.txt"
#define USERNAME_SIZE 32
#define CLUE_SIZE 128

typedef struct {
    int id;
    char username[USERNAME_SIZE];
    float latitude;
    float longitude;
    char clue[CLUE_SIZE];
    int value;
} Treasure;

volatile sig_atomic_t command_ready = 0;

void sigusr1_handler(int sig) {
    command_ready = 1;
}


void list_treasures(const char *hunt_id) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/treasures.dat", hunt_id);

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        perror("fopen");
        return;
    }

    printf("Treasures in hunt '%s':\n", hunt_id);
    Treasure t;
    int i = 0;

    while (fread(&t, sizeof(Treasure), 1, f) == 1) {
        printf("DEBUG raw value bytes: ");
        unsigned char *p = (unsigned char*)&t;
        for (int j = 0; j < sizeof(Treasure); j++) {
            printf("%02X ", p[j]);
        }
        printf("\n");

        printf("ID: %d | User: %s | GPS: (%.4f, %.4f) | Value: %d\n",
               t.id, t.username, t.latitude, t.longitude, t.value);
        i++;
    }

    if (i == 0)
        printf("No treasures found.\n");

    fclose(f);
}



void view_treasure(const char *hunt_id, int treasure_id) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/treasures.dat", hunt_id);

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        perror("Failed to open treasure file");
        return;
    }

    Treasure t;
    int found = 0;

    while (fread(&t, sizeof(Treasure), 1, f) == 1) {
        if (t.id == treasure_id) {
            found = 1;
            printf("Treasure Details (ID: %d):\n", t.id);
            printf("User: %s\n", t.username);
            printf("GPS Coordinates: (%.4f, %.4f)\n", t.latitude, t.longitude);
            printf("Clue: %s\n", t.clue);
            printf("Value: %d\n", t.value);
            break;
        }
    }

    if (!found) {
        printf("Treasure with ID %d not found in hunt '%s'.\n", treasure_id, hunt_id);
    }

    fclose(f);
}

void list_hunts() {
    DIR *dir = opendir("."); 
    if (!dir) {
        perror("Failed to open current directory");
        return;
    }

    struct dirent *entry;
    printf("Listing all hunts:\n");

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char hunt_path[256];
        snprintf(hunt_path, sizeof(hunt_path), "%s/treasures.dat", entry->d_name);

        struct stat st;
        if (stat(hunt_path, &st) == 0) {
            FILE *f = fopen(hunt_path, "rb");
            if (f) {
                int treasure_count = 0;
                Treasure t;

                while (fread(&t, sizeof(Treasure), 1, f) == 1) {
                    treasure_count++;
                }
                fclose(f);

                printf("Hunt: %s | Treasures: %d\n", entry->d_name, treasure_count);
            } else {
                perror("Failed to open treasures.dat");
            }
        }
    }

    closedir(dir); 
}

void process_command() {
    FILE *f = fopen(COMMAND_FILE, "r");
    if (!f) {
        perror("fopen");
        return;
    }

    char command[128];
    if (!fgets(command, sizeof(command), f)) {
        fclose(f);
        return;
    }
    fclose(f);
    command[strcspn(command, "\n")] = 0;

    if (strcmp(command, "stop_monitor") == 0) {
        printf("Stopping monitor as requested.\n");
        exit(0);

    } else if (strncmp(command, "list_treasures", 14) == 0) {
        char hunt_id[64];
        if (sscanf(command, "list_treasures %63s", hunt_id) == 1) {
            list_treasures(hunt_id);
        } else {
            printf("Invalid list_treasures command format.\n");
        }

    } else if (strcmp(command, "list_hunts") == 0) {
        list_hunts();

    } else if (strncmp(command, "view_treasure", 13) == 0) {
        char hunt_id[64];
        int treasure_id;
        if (sscanf(command, "view_treasure %63s %d", hunt_id, &treasure_id) == 2) {
            view_treasure(hunt_id, treasure_id);
        } else {
            printf("Invalid view_treasure command format.\n");
        }

    } else {
        printf("Unknown command received: %s\n", command);
    }
}



int main() {
    struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    setvbuf(stdout, NULL, _IONBF, 0);

    printf("Monitor is running (PID %d). Waiting for commands...\n", getpid());

    while (1) {
        pause();  
        if (command_ready) {
            command_ready = 0;
            process_command();
        }
    }

    return 0;
}
