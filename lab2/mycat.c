#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>

bool got_n = false;
bool got_b = false;
bool got_E = false;

void print_line(char* line) {
    if (got_E) {
        while (*line != '\n') {
            putchar(*line);
            ++line;
        }
        printf("$\n");
    }
    else {
        printf("%s", line);  
    }
}

void print_file(FILE *file) {
    if (file == NULL) {
        fprintf(stderr, "\nUnexpected error: print_file got NULL\n");
        return;
    }
    char* line = NULL;
    size_t len = 0;
    bool is_empty = false;
    int line_number = 1;
    
    while (getline(&line, &len, file) != -1) {
        if (line == NULL) {
            fprintf(stderr, "\nUnexpected error: getline failed to allocate memory\n");
            continue;
        }
        is_empty = (line[0] == '\n');

        if (got_n && (!got_b || !is_empty)) {
            printf("%6d\t", line_number++);
        }
        else if (got_b && !is_empty) {
            printf("%6d\t", line_number++);
        }

        print_line(line);
    }
}

int main(int argc, char *argv[]) {
    int opt;
    
    while ((opt = getopt(argc, argv, "nbE")) != -1) {
        switch (opt) {
            case 'n':
                got_n = true;
                break;
            case 'b':
                got_b = true;
                got_n = false;  
                break;
            case 'E':
                got_E = true;
                break;
            default:
                fprintf(stderr, "\nUsage: %s [-nbE]... [FILE]...\n", argv[0]);
                return 1;
        }
    }
        
    if (optind == argc) {
        print_file(stdin);
    }
    else {
        for (int i = optind; i < argc; i++) {
            FILE *file = fopen(argv[i], "r");
            if (file == NULL) {
                fprintf(stderr, "\nUnexpected error: Cannot read '%s': \n", argv[i]);
                return -1;
            }
            print_file(file);
            fclose(file);
        }
    }

    return 0;
}
