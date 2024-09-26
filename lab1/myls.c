#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <dirent.h>
#define __USE_XOPEN_EXTENDED
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>

#define MYLS_BLUE "\x1b[1;34m"
#define MYLS_GREEN "\x1b[1;32m"
#define MYLS_CYAN "\x1b[1;36m"
#define MYLS_DEFAULT "\x1b[0;39;49m"

#define MYLS_MAX_MODS_LENGTH (10 + 13)
#define MYLS_MAX_FILE_LENGTH (255 + MYLS_MAX_MODS_LENGTH + 1)
#define MYLS_MAX_PATH_LENGTH (4096 + 1)
#define MYLS_MAX_USER_LENGTH (32 + 1)

enum MYLS_WIDTH {
    PERMISSIONS = 10,
    MONTH = 3,
    DATE = 2,
    TIME = 5
};

enum MYLS_WIDTH_INDEX {
    HLINKS = 0,
    USERNAME = 1,
    GROUPNAME = 2,
    SIZE = 3
};

struct entry {
    char name[MYLS_MAX_FILE_LENGTH];
    char permissions[PERMISSIONS + 1];
    size_t hlinks;
    char username[MYLS_MAX_USER_LENGTH];
    char groupname[MYLS_MAX_USER_LENGTH];
    size_t size;
    char month[MONTH + 1];
    unsigned char date;
    char time[TIME + 1];
    bool have_username;
    bool have_groupname;
};

bool got_a = false, got_l = false;
size_t curr_year = 2024;

int filter(const struct dirent* entry) {
    if (entry == NULL) {
        fprintf(stderr, "\nUnexpected error: filter got NULL\n");
        return 0;
    }
    if (got_a) {
        return 1;
    }
    return entry->d_name[0] != '.';
}

int compare(const struct dirent** first, const struct dirent** second) {
    if (first == NULL || second == NULL) {
        fprintf(stderr, "\nUnexpected error: compare got %p, %p\n", first, second);
        return 0;
    }
    char* first_str = (char*)((*first)->d_name);
    char* second_str = (char*)((*second)->d_name);
    int first_delete = 0, second_delete = 0;
    while (first_str[0] == '.') {
        first_str++;
        first_delete++;
    }
    while (second_str[0] == '.') {
        second_str++;
        second_delete++;
    }
    int cmp = strcasecmp(first_str, second_str);
    if (cmp == 0) {
        return first_delete - second_delete;
    }
    return cmp;
}

char file_type(mode_t mode) {
    return S_ISREG(mode) ? '-': S_ISDIR(mode) ? 'd':
    S_ISLNK(mode) ? 'l': S_ISCHR(mode) ? 'c': S_ISBLK(mode) ? 'b':
    __S_ISTYPE(mode, __S_IFSOCK) ? 's': S_ISFIFO(mode) ? 'p' : '!';
}

size_t count_digits(size_t num) {
    size_t count = 0;
    if (num == 0) {
        return 1;
    }
    while (num > 0) {
        num /= 10;
        count++;
    }
    return count;
}

int process_entries(
    const char *const dir, struct dirent **const old_array, size_t entries_number,
    struct entry *const new_array, unsigned int *const max_widths, size_t *const blocks) {
    if (dir == NULL || old_array == NULL || new_array == NULL || max_widths == NULL || blocks == NULL) {
        fprintf(stderr, "\nUnexpected error: process_entries got %p, %p, %p, %p, %p\n",
        dir, old_array, new_array, max_widths, blocks);
        return -1;
    }
    char path[MYLS_MAX_PATH_LENGTH];
    struct stat file_data;
    struct passwd* user;
    struct group* group;
    struct tm* time;
    for (size_t i = 0; i < entries_number; i++) {
        sprintf(path, "%s/%s", dir, old_array[i]->d_name);
        if (lstat(path, &file_data) == -1) {
            fprintf(stderr, "\nUnexpected error: Cannot read '%s': \n", path);
            return -1;
        }
        sprintf(new_array[i].name, "%s", old_array[i]->d_name);
        sprintf(new_array[i].permissions, "%c%c%c%c%c%c%c%c%c%c", file_type(file_data.st_mode),
        file_data.st_mode & S_IRUSR ? 'r': '-', file_data.st_mode & S_IWUSR ? 'w': '-', file_data.st_mode & S_IXUSR ? 'x': '-',
        file_data.st_mode & S_IRGRP ? 'r': '-', file_data.st_mode & S_IWGRP ? 'w': '-', file_data.st_mode & S_IXGRP ? 'x': '-',
        file_data.st_mode & S_IROTH ? 'r': '-', file_data.st_mode & S_IWOTH ? 'w': '-', file_data.st_mode & S_IXOTH ? 'x': '-');
        new_array[i].hlinks = file_data.st_nlink;

        user = getpwuid(file_data.st_uid);
        group = getgrgid(file_data.st_gid);
        if (user == NULL || user->pw_name == NULL) {
            new_array[i].have_username = false;
            sprintf(new_array[i].username, "%u", file_data.st_uid);
        }
        else {
            new_array[i].have_username = true;
            sprintf(new_array[i].username, "%s", user->pw_name);
        }
        if (group == NULL || group->gr_name == NULL) {
            new_array[i].have_groupname = false;
            sprintf(new_array[i].groupname, "%u", file_data.st_gid);
        }
        else {
            new_array[i].have_groupname = true;
            sprintf(new_array[i].groupname, "%s", group->gr_name);
        }

        new_array[i].size = file_data.st_size;
        time = localtime(&file_data.st_mtime);
        if (time == NULL) {
            fprintf(stderr, "\nUnexpected error: Cannot extract time\n");
        }
        strftime(new_array[i].month, MONTH + 1, "%b", time);
        new_array[i].date = time->tm_mday;
        if (time->tm_year == curr_year) {
            strftime(new_array[i].time, TIME + 1, "%H:%M", time);
        }
        else {
            strftime(new_array[i].time, TIME + 1, "%Y", time);
        }

        *blocks += file_data.st_blocks;

        if (S_ISDIR(file_data.st_mode)) {
            sprintf(new_array[i].name, "%s%s%s", MYLS_BLUE, old_array[i]->d_name, MYLS_DEFAULT);
        }
        else if (S_ISREG(file_data.st_mode) && ((S_IXUSR | S_IXGRP | S_IXOTH) & file_data.st_mode)) {
            sprintf(new_array[i].name, "%s%s%s", MYLS_GREEN, old_array[i]->d_name, MYLS_DEFAULT);
        }
        else if (S_ISLNK(file_data.st_mode)) {
            if (stat(path, &file_data) == -1) {
                fprintf(stderr, "\nUnexpected error: Cannot read '%s': \n", path);
                continue;
            }
            char link[MYLS_MAX_PATH_LENGTH];
            readlink(path, link, MYLS_MAX_PATH_LENGTH - 1);

            char link_modified[MYLS_MAX_PATH_LENGTH + MYLS_MAX_MODS_LENGTH];
            if (S_ISDIR(file_data.st_mode)) {
                sprintf(link_modified, "%s%s%s", MYLS_BLUE, link, MYLS_DEFAULT);
            }
            else if (S_ISREG(file_data.st_mode) && ((S_IXUSR | S_IXGRP | S_IXOTH) & file_data.st_mode)) {
                sprintf(link_modified, "%s%s%s", MYLS_GREEN, link, MYLS_DEFAULT);
            }
            else if (S_ISLNK(file_data.st_mode)) {
                sprintf(link_modified, "%s%s%s", MYLS_CYAN, link, MYLS_DEFAULT);
            }
            else {
                sprintf(link_modified, "%s", link);
            }

            sprintf(new_array[i].name, "%s%s%s -> %s", MYLS_CYAN, old_array[i]->d_name, MYLS_DEFAULT, link_modified);
        }
        else {
            sprintf(new_array[i].name, "%s", old_array[i]->d_name);
        }

        size_t len = count_digits(new_array[i].hlinks);
        if (len > max_widths[0]) {
            max_widths[0] = len;
        }
        len = strlen(new_array[i].username);
        if (len > max_widths[1]) {
            max_widths[1] = len;
        }
        len = strlen(new_array[i].groupname);
        if (len > max_widths[2]) {
            max_widths[2] = len;
        }
        len = count_digits(new_array[i].size);
        if (len > max_widths[3]) {
            max_widths[3] = len;
        }
    }
}

void print_line(const struct entry *const entry, const unsigned int *const max_widths) {
    if (entry->have_username) {
        if (entry->have_groupname) {
            printf("%-*s %*llu %-*s %-*s %*llu %-*s %*hhu %*s %-s\n",
            PERMISSIONS, entry->permissions, max_widths[HLINKS], entry->hlinks,
            max_widths[USERNAME], entry->username, max_widths[GROUPNAME], entry->groupname,
            max_widths[SIZE], entry->size, MONTH, entry->month, DATE, entry->date, TIME, entry->time, entry->name);
        }
        else {
            printf("%-*s %*llu %-*s %*s %*llu %-*s %*hhu %*s %-s\n",
            PERMISSIONS, entry->permissions, max_widths[HLINKS], entry->hlinks,
            max_widths[USERNAME], entry->username, max_widths[GROUPNAME], entry->groupname,
            max_widths[SIZE], entry->size, MONTH, entry->month, DATE, entry->date, TIME, entry->time, entry->name);
        }
    }
    else {
        if (entry->have_groupname) {
            printf("%-*s %*llu %*s %-*s %*llu %-*s %*hhu %*s %-s\n",
            PERMISSIONS, entry->permissions, max_widths[HLINKS], entry->hlinks,
            max_widths[USERNAME], entry->username, max_widths[GROUPNAME], entry->groupname,
            max_widths[SIZE], entry->size, MONTH, entry->month, DATE, entry->date, TIME, entry->time, entry->name);
        }
        else {
            printf("%-*s %*llu %*s %*s %*llu %-*s %*hhu %*s %-s\n",
            PERMISSIONS, entry->permissions, max_widths[HLINKS], entry->hlinks,
            max_widths[USERNAME], entry->username, max_widths[GROUPNAME], entry->groupname,
            max_widths[SIZE], entry->size, MONTH, entry->month, DATE, entry->date, TIME, entry->time, entry->name);
        }
    }
}

int print_entry(const char *const dir, const char *const file) {
    if (dir == NULL || file == NULL) {
        fprintf(stderr, "\nUnexpected error: print_entry got %p, %p\n", dir, file);
        return -1;
    }
    char path[MYLS_MAX_PATH_LENGTH];
    sprintf(path, "%s/%s", dir, file);
    struct stat file_data;
    if (lstat(path, &file_data) == -1) {
        fprintf(stderr, "\nUnexpected error: Cannot read '%s': \n", path);
        return -1;
    }
    if (S_ISDIR(file_data.st_mode)) {
        printf("%s%s%s  ", MYLS_BLUE, file, MYLS_DEFAULT);
    }
    else if (S_ISREG(file_data.st_mode) && ((S_IXUSR | S_IXGRP | S_IXOTH) & file_data.st_mode)) {
        printf("%s%s%s  ", MYLS_GREEN, file, MYLS_DEFAULT);
    }
    else if (S_ISLNK(file_data.st_mode)) {
        printf("%s%s%s  ", MYLS_CYAN, file, MYLS_DEFAULT);
    }
    else {
        printf("%s  ", file);
    }
    return 0;
}

int print_dir(const char *const name) {
    if (name == NULL) {
        fprintf(stderr, "\nUnexpected error: print_dir got NULL\n");
        return -1;
    }

    struct dirent** entries_array = NULL;
    int entries_number = scandir(name, &entries_array, filter, compare);
    if (entries_number == -1) {
        fprintf(stderr, "\nmyls: cannot access '%s': No such file or directory\n", name);
        return -1;
    }

    struct entry* entries_array_fine;
    unsigned int aligns[4] = {1, 1, 1, 1};
    size_t blocks = 0;
    if (!got_l) {
        for (size_t i = 0; i < entries_number; i++) {
            print_entry(name, entries_array[i]->d_name);
        }
        printf("\n");
    }
    else {
        entries_array_fine = (struct entry*)calloc(entries_number, sizeof(struct entry));
        if (entries_array_fine == NULL) {
            fprintf(stderr, "\nAllocation error\n");
            return -1;
        }
        process_entries(name, entries_array, entries_number, entries_array_fine, aligns, &blocks);
        printf("total %llu\n", blocks / 2);
        for (size_t i = 0; i < entries_number; i++) {
            print_line(&entries_array_fine[i], aligns);
        }

        free(entries_array_fine);
    }

    for (size_t i = 0; i < entries_number; i++) {
        free(entries_array[i]);
    }
    free(entries_array);    
    return 0;
}

void set_actual_time() {
    time_t raw_curr_time = time(NULL);
    struct tm* curr_time = localtime(&raw_curr_time);
    curr_year = curr_time->tm_year;
}

int main(int argc, char **argv)
{
    set_actual_time();
    int opt;
    while ((opt = getopt(argc, argv, "la")) != -1)
    {
        switch (opt)
        {
        case 'l':
            got_l = true;
            break;
        case 'a':
            got_a = true;
            break;

        default:
            break;
        }
    }

    if (optind == argc) {
        print_dir(".");
    }
    else if (argc - optind == 1) {
        print_dir(argv[optind]);
    }
    else {
        for (int i = optind; i < argc - 1; i++) {
            printf("%s:\n", argv[i]);
            print_dir(argv[i]);
            printf("\n");
        }
        printf("%s:\n", argv[argc - 1]);
        print_dir(argv[argc - 1]);
    }
        
    return 0;
}
