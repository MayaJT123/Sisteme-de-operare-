
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>


#define USERNAME_SIZE 32
#define CLUE_SIZE 128
#define BUFFER_SIZE 256

typedef struct {
    int id;
    char username[USERNAME_SIZE];
    float latitude;
    float longitude;
    char clue[CLUE_SIZE];
    int value;
} Treasure;

char* build_path(const char* hunt_id, const char* filename) {
    static char path[BUFFER_SIZE];
    snprintf(path, BUFFER_SIZE, "%s/%s", hunt_id, filename);
    return path;
}

void log_operation(const char* hunt_id, const char* message) {
    char* log_path = build_path(hunt_id, "logged_hunt");
    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return;

    time_t now = time(NULL);
    char* timestr = ctime(&now);
    timestr[strlen(timestr) - 1] = '\0'; // remove newline

    dprintf(fd, "[%s] %s\n", timestr, message);
    close(fd);
}

void create_symlink(const char* hunt_id) {
    char link_name[BUFFER_SIZE];
    snprintf(link_name, BUFFER_SIZE, "logged_hunt-%s", hunt_id);
    char* target = build_path(hunt_id, "logged_hunt");
    symlink(target, link_name); // Ignore error if already exists
}

void add_treasure(const char* hunt_id) {
    mkdir(hunt_id, 0755);
    char* path = build_path(hunt_id, "treasures.dat");
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) { perror("open"); return; }

    Treasure t;
    printf("Enter treasure ID: "); scanf("%d", &t.id);
    printf("Enter username: "); scanf("%s", t.username);
    printf("Enter latitude: "); scanf("%f", &t.latitude);
    printf("Enter longitude: "); scanf("%f", &t.longitude);
    printf("Enter clue: "); getchar();
    fgets(t.clue, CLUE_SIZE, stdin);
    t.clue[strcspn(t.clue, "\n")] = 0;
    printf("Enter value: "); scanf("%d", &t.value);

    write(fd, &t, sizeof(Treasure));
    close(fd);

    log_operation(hunt_id, "add treasure");
    create_symlink(hunt_id);
}

void list_treasures(const char* hunt_id) {
    char* path = build_path(hunt_id, "treasures.dat");
    struct stat st;
    if (stat(path, &st) == -1) { perror("stat"); return; }

    printf("Hunt: %s\n", hunt_id);
    printf("File size: %ld bytes\n", st.st_size);
    printf("Last modified: %s", ctime(&st.st_mtime));

    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open"); return; }
    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %d | User: %s | GPS: (%.4f, %.4f) | Value: %d\n",
            t.id, t.username, t.latitude, t.longitude, t.value);
    }
    close(fd);
}

void view_treasure(const char* hunt_id, int target_id) {
    char* path = build_path(hunt_id, "treasures.dat");
    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open"); return; }
    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id == target_id) {
            printf("ID: %d\nUsername: %s\nLat: %.4f\nLon: %.4f\nClue: %s\nValue: %d\n",
                   t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
            close(fd);
            return;
        }
    }
    printf("Treasure with ID %d not found.\n", target_id);
    close(fd);
}

void remove_treasure(const char* hunt_id, int target_id) {
    char* path = build_path(hunt_id, "treasures.dat");
    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open"); return; }

    int temp_fd = open("temp.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd < 0) { perror("temp file"); close(fd); return; }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id != target_id)
            write(temp_fd, &t, sizeof(Treasure));
    }
    close(fd); close(temp_fd);
    rename("temp.dat", path);

    log_operation(hunt_id, "remove_treasure");
}

void remove_hunt(const char* hunt_id) {
    DIR* dir = opendir(hunt_id);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent* entry;
    char filepath[BUFFER_SIZE];

    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(filepath, BUFFER_SIZE, "%s/%s", hunt_id, entry->d_name);
        if (unlink(filepath) != 0) {
            perror("unlink");
        }
    }

    closedir(dir);

    if (rmdir(hunt_id) != 0) {
        perror("rmdir");
    }

    char link[BUFFER_SIZE];
    snprintf(link, BUFFER_SIZE, "logged_hunt-%s", hunt_id);
    unlink(link);

    printf("Hunt '%s' and its contents were removed.\n", hunt_id);
}


int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s --<operation> <hunt_id> [<id>]\n", argv[0]);
        return 1;
    }

    const char* op = argv[1];
    const char* hunt_id = argv[2];

    if (strcmp(op, "--add") == 0) {
        add_treasure(hunt_id);
    } else if (strcmp(op, "--list") == 0) {
        list_treasures(hunt_id);
    } else if (strcmp(op, "--view") == 0 && argc == 4) {
        view_treasure(hunt_id, atoi(argv[3]));
     } else if (strcmp(op, "--remove_treasure") == 0 && argc == 4) {
       remove_treasure(hunt_id, atoi(argv[3]));
     } else if (strcmp(op, "--remove_hunt") == 0) {
       remove_hunt(hunt_id);
    } else {
     fprintf(stderr, "Invalid command or arguments.\n");
    }

    return 0;
}
