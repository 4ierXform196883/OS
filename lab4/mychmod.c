#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#define bool	_Bool
#define true	1
#define false	0

enum UsersCategory {
    USER = 0,
    GROUP,
    OTHER
};
enum Permissions {
    READ = 0,
    WRITE,
    EXECUTE
};

int process_symbolic_form(const char* mode_str, mode_t* old_mode) {
    char buffer[256];
    buffer[0] = '\0';
    sscanf(mode_str, "%[^=+-]", buffer);
    buffer[255] = '\0';

    bool targets[3] = {false, false, false};

    int i = 0;
    while (buffer[i] != '\0') {
        switch (buffer[i]) {
        case 'u':
            targets[USER] = true;
            break;
        case 'g':
            targets[GROUP] = true;
            break;
        case 'o':
            targets[OTHER] = true;
            break;
        case 'a':
            targets[USER] = true;
            targets[GROUP] = true;
            targets[OTHER] = true;
            break;
        default:
            return -1;
        }
        i++;
    }
    if (i == 0) {
        targets[USER] = true;
        targets[GROUP] = true;
        targets[OTHER] = true;
    }

    sscanf(mode_str + i, "%[=+-]", buffer);
    buffer[255] = '\0';

    if (buffer[1] != '\0') {
        return -1;
    }

    char operation = buffer[0];

    i++;

    bool permissions[3] = {false, false, false};
    int source = -1;
    if (mode_str[i] == 'u') {
        source = USER;
    }
    else if (mode_str[i] == 'g') {
        source = GROUP;
    }
    else if (mode_str[i] == 'o') {
        source = OTHER;
    }

    if (source != -1) {
        if (mode_str[i + 1] != '\0') {
            return -1;
        }
        if (source == USER) {
            if (S_IRUSR & *old_mode) {
                permissions[READ] = true;
            }
            if (S_IWUSR & *old_mode) {
                permissions[WRITE] = true;
            }
            if (S_IXUSR & *old_mode) {
                permissions[EXECUTE] = true;
            }
        }
        else if (source == GROUP) {
            if (S_IRGRP & *old_mode) {
                permissions[READ] = true;
            }
            if (S_IWGRP & *old_mode) {
                permissions[WRITE] = true;
            }
            if (S_IXGRP & *old_mode) {
                permissions[EXECUTE] = true;
            }
        }
        else if (source == OTHER) {
            if (S_IROTH & *old_mode) {
                permissions[READ] = true;
            }
            if (S_IWOTH & *old_mode) {
                permissions[WRITE] = true;
            }
            if (S_IXOTH & *old_mode) {
                permissions[EXECUTE] = true;
            }
        }
    }

    while (mode_str[i] != '\0') {
        switch (mode_str[i]) {
        case 'r':
            permissions[READ] = true;
            break;
        case 'w':
            permissions[WRITE] = true;
            break;
        case 'x':
            permissions[EXECUTE] = true;
            break;
        default:
            return -1;
        }
        i++;
    }

    mode_t change = 0;
    if (targets[USER]) {
        change |= ((permissions[READ] ? S_IRUSR: 0) | (permissions[WRITE] ? S_IWUSR: 0) | (permissions[EXECUTE] ? S_IXUSR: 0));
    }
    if (targets[GROUP]) {
        change |= ((permissions[READ] ? S_IRGRP: 0) | (permissions[WRITE] ? S_IWGRP: 0) | (permissions[EXECUTE] ? S_IXGRP: 0));
    }
    if (targets[OTHER]) {
        change |= ((permissions[READ] ? S_IROTH: 0) | (permissions[WRITE] ? S_IWOTH: 0) | (permissions[EXECUTE] ? S_IXOTH: 0));
    }

    if (operation == '=') {
        *old_mode = change;
    }
    else if (operation == '+') {
        *old_mode |= change;
    }
    else if (operation == '-') {
        *old_mode &= (~change);
    }
    
    return 0;
}

int process_octal_form(const char* mode_str, mode_t* new_mode) {
    mode_t octal = 0;
    for (int i = 1; i < 3; i++) {
        if (mode_str[i] < '0' || mode_str[i] > '7') {
            return -1;
        }
        else {
            octal *= (mode_t)8;
            octal += (mode_t)(mode_str[i] - '0');
        }
    }
    if (mode_str[3] != '\0') {
        return -1;
    }
    *new_mode = octal;

    return 0;
}

int change_mode(const char *mode_str, const char *file_name) {
    if (mode_str == NULL || file_name == NULL) {
        fprintf(stderr, "Unexpected error: change_mode got %p, %p\n", mode_str, file_name);
        return -1;
    }
    struct stat file_stat;

    if (stat(file_name, &file_stat) != 0) {
        perror("stat() failed");
        return -1;
    }

    mode_t mode = file_stat.st_mode;

    if (isdigit(mode_str[0])) {
        if (process_octal_form(mode_str, &mode) != 0) {
            fprintf(stderr, "mychmod: invalid mode: %s\n", mode_str);
            return -1;
        }

        if (chmod(file_name, mode) != 0) {
            perror("chmod() failed");
            return -1;
        }
    }
    else
    {
        if (process_symbolic_form(mode_str, &mode) != 0) {
            fprintf(stderr, "mychmod: invalid mode: %s\n", mode_str);
            return -1;
        }

        if (chmod(file_name, mode) != 0) {
            perror("chmod() failed");
            return -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s [OPTION]... MODE[,MODE]... FILE...\n or:  %s OCTAL-MODE FILE...\n", argv[0], argv[0]);
        return 1;
    }

    const char *mode_str = argv[1];
    const char *file_name = argv[2];

    if (change_mode(mode_str, file_name) != 0) {
        return 1;
    }

    return 0;
}
