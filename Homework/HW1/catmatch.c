#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAXSIZE 255

int main(int argc, char *argv[], char *envp[]) {

    //I COMPLETED THE TMUX EXERCISE
    
    int pid = getpid();
    fprintf(stderr, "%d\n\n", pid);

    char *filename = argv[1];
    char *pattern = getenv("CATMATCH_PATTERN");

    FILE *fp;
    fp = fopen(filename, "r");

    if (fp == NULL) {
       fprintf(stderr, "[ERROR] File could not be opened\n");
       return 1;
    }

    char line[MAXSIZE+2];

    while (fgets(line, MAXSIZE, fp) != NULL) {
       if (strstr(line, pattern) != NULL) {
           printf("1 ");
       }
       else {
           printf("0 ");
       }
       printf("%s", line);
    }


    return 0;
}
