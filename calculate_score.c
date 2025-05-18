#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#define MAX_USER 64

typedef struct {
    int id;
    char username[32];
    float lat, lon;
    char clue[128];
    int value;
} Treasure;

typedef struct Node {
    char user[32];
    int score;
    struct Node *next;
} ScoreNode;

ScoreNode* find_or_add_user(ScoreNode **head, const char *username) {
    ScoreNode *curent = *head;
    while (curent) {
        if (strcmp(curent->user, username) == 0) return curent;
        curent = curent->next;
    }
    ScoreNode *new_node = malloc(sizeof(ScoreNode));

    if (!new_node) {
        perror("malloc");
        exit(1);
    }
    
    strcpy(new_node->user, username);
    new_node->score = 0;
    new_node->next = *head;
    *head = new_node;
    return new_node;
}

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;
    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", argv[1]);

    FILE *f = fopen(path, "rb");
    if (!f) return 1;

    ScoreNode *scores = NULL;
    Treasure t;
    while (fread(&t, sizeof(Treasure), 1, f) == 1) {
        ScoreNode *node = find_or_add_user(&scores, t.username);
        node->score += t.value;
    }
    fclose(f);

    ScoreNode *curent = scores;
    while (curent) {
        printf("%s: %d\n", curent->user, curent->score);
        ScoreNode *tmp = curent;
        curent = curent->next;
        free(tmp);
    }
    return 0;
}
