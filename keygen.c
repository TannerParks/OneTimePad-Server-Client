#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[]) {
    int length = atoi(argv[1]);
    char *chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";  // Alphabet + space
    char key[length+1];
    int random;

    for (int i=0; i<length; i++) {
        //random = (rand() % 27);
        //printf("%c", random+65);
        key[i] = chars[rand() % 27];
    }

    key[length] = '\0';

    printf("%s\n", key);

    return 0;
}