#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <regex.h>
#include <stdbool.h>
#include <string.h>

#define MYGREP_RED "\x1b[1;31m"
#define MYGREP_DEFAULT "\x1b[0;39;49m"

void highlighted_print(const char *line, regex_t *regex) {
    regmatch_t match;
    bool no_matches = true;

    while (regexec(regex, line, 1, &match, 0) == 0) {
        no_matches = false;
        if (match.rm_eo - match.rm_so == 0) {
            break;
        }
        fwrite(line, 1, match.rm_so, stdout);
        fwrite(MYGREP_RED, 1, strlen(MYGREP_RED), stdout);
        fwrite(line + match.rm_so, 1, match.rm_eo - match.rm_so, stdout);
        fwrite(MYGREP_DEFAULT, 1, strlen(MYGREP_DEFAULT), stdout);
        line += match.rm_eo;
    }
    if (!no_matches) {
        fputs(line, stdout);
    }
}

void search_in_file(FILE *file, regex_t *regex)
{
    if (file == NULL || regex == NULL) {
        fprintf(stderr, "\nUnexpected error: search_in_file got %p, %p\n", file, regex);
        return;
    }

    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, file) != -1) {
        if (line == NULL) {
            fprintf(stderr, "\nUnexpected error: getline failed to allocate memory\n");
            continue;
        }
        highlighted_print(line, regex);
    }

    free(line);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "\nUsage: %s pattern [FILE]...\n", argv[0]);
        return 1;
    }

    regex_t regex;
    if (regcomp(&regex, argv[1], REG_EXTENDED) != 0) {
        fprintf(stderr, "\nmygrep: wrong regex in pattern: %s\n", argv[1]);
        return -1;
    }

    if (argc == 2) {
        search_in_file(stdin, &regex);
    }
    else {
        for (int i = 2; i < argc; i++) {
            FILE *file = fopen(argv[i], "r");
            if (file == NULL) {
                fprintf(stderr, "\nmygrep: Cannot read '%s'\n", argv[i]);
                continue;
            }
            search_in_file(file, &regex);
            fclose(file);
        }
    }

    regfree(&regex);
    return 0;
}
