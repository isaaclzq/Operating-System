#include <stdio.h>
#include <stdlib.h>
void count(char* file){
    FILE* filePointer = fopen(file, "r");
    if (!filePointer){
        printf("Error: the file %s is invalid\n", file);
    } else {
        char* line = NULL;
        size_t len = 0;
        ssize_t read = 0;
        ssize_t num_line = 0, num_character = 0, num_word = 0;
        ssize_t whitespace = 0;
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
            whitespace = 0;
        }
        fclose(filePointer);
        printf("    %ld    %ld    %ld    %s\n", num_line, num_word, num_character, file); 
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1){
       printf("Error: Require a file\n");
       return 0;
    }
    char* file = argv[1];
    count(file);
    return 0;
}
