#include <stdio.h>
#include <stdlib.h>


void usage(char *name) {
    printf("%s <seed> <num clients> <server out> <clients out>\n", name);
}


int main(int argc, char *argv[]) {
    if(argc < 5) {
        usage(argv[0]);
        return -1;
    }

    int seed = strtol(argv[1], NULL, 10),
        clients = strtol(argv[2], NULL, 10);
    if(seed <= 0 || clients <= 0 || argc != clients + 4) {
        usage(argv[0]);
        return -2;
    }

    int i, j;
    
    // open output files
    FILE **fds = malloc((clients + 1) * sizeof(FILE *));
    fds[0] = fopen(argv[3], "w");
    for(i = 0; i < clients; i++)
        fds[i + 1] = fopen(argv[4 + i], "w");

    for(i = 0; i < 100; i++) {
        int x = rand() % 100,
            y = rand() % 100,
            correct = x + y;

        // server quits with probability 0.05
        if(rand() % 100 < 5)
            fprintf(fds[0], "-1\n2\n");
        else
            fprintf(fds[0], "%d\n%d\n", x, y);

        // generate some answers until one is correct
        int someone_correct = 0;
        while(!someone_correct) {
            for(j = 0; j < clients; j++) {
                int sleep = rand() % 5, answer;

                // correct answer with probability 0.2
                if(rand() % 100 < 20) {
                    someone_correct = 1;
                    answer = correct;
                }
                else answer = rand() % 100;
                fprintf(fds[1 + j], "sleep %d\n%d\n", sleep, answer);

                // client quits with probability 0.05
                if(rand() % 100 < 5)
                    fprintf(fds[1 + j], "signal 2\n");
            }
        }
    }

    // close files
    fclose(fds[0]);
    for(i = 0; i < clients; i++)
        fclose(fds[1 + i]);

    return 0;
}
