#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define BUF_SIZE 512
#define BUF_MIN 128

void countInput(){
    char *input, *p;
    int len, remain, n, size;
    int x = 0;
    int num_line = 0, num_character = 0, num_word = 0;
    size = BUF_SIZE;
    input = malloc(size);
    len = 0;
    remain = size;
    while (!feof(stdin)) {
        if (remain <= BUF_MIN) {
            remain += size;
            size *= 2;
            p = realloc(input, size);
            if (p == NULL) {
                free(input);
                exit(0);
            }
            input = p;
        }
        fgets(input + len, remain, stdin);
        n = strlen(input + len);
        len += n;
        remain -= n;
    }
    //printf("the length of the string is %d\n", len);
    for (; x < len; x++){
        if (input[x] == '\n'){
            num_line++;
        }
    }
    for (x = 0; x < len-1; x++){
        if ((isalnum(input[x]) || ispunct(input[x])) && input[x+1] == ' '){
            num_word++;
        }
        if ((isalnum(input[x]) || ispunct(input[x])) && input[x+1] == '\n'){
            num_word++;
        }
    }
    num_character += len;
    printf("    %d    %d    %d\n", num_line, num_word, num_character); 
    free(input);
}

void count(char* file){
    FILE* filePointer = fopen(file, "r");
    if (!filePointer){
    	countInput();
    } else {
        char* line = NULL;
        size_t len = 0;
        ssize_t read = 0;
        ssize_t num_line = 0, num_character = 0, num_word = 0;
        while ((read = getline(&line, &len, filePointer)) != -1){
            num_line++;
            num_character += read;
            int x = 0;
            for (; x < read-1; x++){
                if (line[x] != ' ' && line[x+1] == ' '){
                    num_word++;
                } 
                if (line[x] != ' ' && line[x+1] == '\n'){
                    num_word++;
                }
            }
        }
        fclose(filePointer);
        printf("    %ld    %ld    %ld    %s\n", num_line, num_word, num_character, file); 
    }
}

int main(int argc, char *argv[]) {
    // if (argc == 1){
    //    printf("Error: Require a file\n");
    //    return 0;
    // }
    // printf("%d\n", argc);
    char* file = argv[1];
    count(file);
    return 0;
}
