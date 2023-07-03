#define _GNU_SOURCE

#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <linux/limits.h>

#define VERSION "1.0.0"

typedef unsigned long long ull;

void clean_recursive (const char* path, ull stats_now[3], ull stats_cfg[3]) {
    DIR* dir;
    struct stat attr;
    struct dirent* entry;
    dir = opendir(path);
    if (dir != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
                char path_child[PATH_MAX];
                snprintf(path_child, PATH_MAX, "%s/%s", path, entry->d_name);
                if (entry->d_type == DT_DIR) {
                    clean_recursive(path_child, stats_now, stats_cfg);
                    if (rmdir(path_child) == 0) {
                        stats_now[2] ++;
                        stats_cfg[2] ++;
                    }
                } else {
                    if (stat(path_child, &attr) == 0) {
                        if (unlink(path_child) == 0) {
                            stats_now[0] += attr.st_size;
                            stats_cfg[0] += attr.st_size;
                            stats_now[1] ++;
                            stats_cfg[1] ++;
                        }
                    }
                }
            }
        }
        closedir(dir);
    }
}

char* convert_size (ull size) {
    static char size_str[64];
    if (size < 1024) {
        snprintf(size_str, 64, "%lld B", size);
    } else if (size < pow(1024, 2)) {
        snprintf(size_str, 64, "%.1f KB", size / (double)1024);
    } else if (size < pow(1024, 3)) {
        snprintf(size_str, 64, "%.1f MB", size / (double)pow(1024, 2));
    } else if (size < pow(1024, 4)) {
        snprintf(size_str, 64, "%.1f GB", size / (double)pow(1024, 3));
    } else {
        snprintf(size_str, 64, "%.1f TB", size / (double)pow(1024, 4));
    }
    return size_str;
}

int handle_config (const char* cfg_path, ull stats_any[5][3], bool save) {
    int bcount;
    int mode = S_IRUSR | S_IWUSR;
    int flags = save ? O_WRONLY | O_CREAT | O_TRUNC : O_RDONLY;
    int cfg_fd = open(cfg_path, flags, mode);
    if (cfg_fd == -1) {
        return 1;
    }
    if (save) {
        bcount = write(cfg_fd, stats_any, sizeof(ull) * 15);
    } else {
        bcount = read(cfg_fd, stats_any, sizeof(ull) * 15);
    }
    if (bcount != sizeof(ull) * 15) {
        close(cfg_fd);
        return 1;
    }
    close(cfg_fd);
    return 0;
}

void print_usage (char* basename) {
    printf("swish v%s\n\n", VERSION);
    printf("Usage:\n");
    printf("%s <commands>\n\n", basename);
    printf("Available commands:\n");
    printf("u - Clean user cache (~/.cache)\n");
    printf("s - Clean system cache (/var/cache)\n");
    printf("l - Clean system logs (/var/log)\n");
    printf("t - Clean short-term temporary files (/tmp)\n");
    printf("g - Clean long-term temporary files (/var/tmp)\n");
    printf("c - View current run statistics\n");
    printf("a - View all-time statistics\n");
    printf("r - Reset all-time statistics\n");
}

void print_stats (ull stats_any[5][3]) {
    printf("%-20s", "Freed");
    printf("%-20s", "User Cache");
    printf("%-20s", "System Cache");
    printf("%-20s", "System Logs");
    printf("%-20s", "S-term tmpfiles");
    printf("%-20s\n", "L-term tmpfiles");
    for (int i = 0; i < 3; i++) {
        if (i == 0) {
            printf("%-20s", "Space");
        } else if (i == 1) {
            printf("%-20s", "Files");
        } else {
            printf("%-20s", "Directories");
        }
        for (int j = 0; j < 5; j++) {
            if (i == 0) {
                printf("%-20s", convert_size(stats_any[j][i]));
            } else {
                printf("%-20lld", stats_any[j][i]);
            }
        }
        printf("\n");
    }
    printf("\n");
}

int main(int argc, char** argv) {

    char cfg_path[PATH_MAX];
    char clean_path[PATH_MAX];
    ull stats_now[5][3];
    ull stats_cfg[5][3];

    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }

    snprintf(cfg_path, PATH_MAX, "/home/%s/.config/swish.cfg", getlogin());

    memset(stats_now, 0, sizeof(ull) * 15);

    if (handle_config(cfg_path, stats_cfg, false) != 0) {
        memset(stats_cfg, 0, sizeof(ull) * 15);
    }

    for (int i = 0; argv[1][i] != '\0'; i++) {
        if (argv[1][i] == 'u') {
            snprintf(clean_path, PATH_MAX, "/home/%s/.cache", getlogin());
            printf("[*] Cleaning user cache (%s)...\n", clean_path);
            clean_recursive(clean_path, stats_now[0], stats_cfg[0]);
        } else if (argv[1][i] == 's') {
            snprintf(clean_path, PATH_MAX, "/var/cache");
            printf("[*] Cleaning system cache (%s)...\n", clean_path);
            clean_recursive(clean_path, stats_now[1], stats_cfg[1]);
        } else if (argv[1][i] == 'l') {
            snprintf(clean_path, PATH_MAX, "/var/log");
            printf("[*] Cleaning system logs (%s)...\n", clean_path);
            clean_recursive(clean_path, stats_now[2], stats_cfg[2]);
        } else if (argv[1][i] == 't') {
            snprintf(clean_path, PATH_MAX, "/tmp");
            printf("[*] Cleaning short-term temporary files (%s)...\n", clean_path);
            clean_recursive(clean_path, stats_now[3], stats_cfg[3]);
        } else if (argv[1][i] == 'g') {
            snprintf(clean_path, PATH_MAX, "/var/tmp");
            printf("[*] Cleaning long-term temporary files (%s)...\n", clean_path);
            clean_recursive(clean_path, stats_now[4], stats_cfg[4]);
        } else if (argv[1][i] == 'c') {
            printf("\nCurrent run stats:\n");
            print_stats(stats_now);
        } else if (argv[1][i] == 'a') {
            printf("\nAll-time stats:\n");
            print_stats(stats_cfg);
        } else if (argv[1][i] == 'r') {
            printf("[*] Resetting all-time statistics...\n");
            memset(stats_cfg, 0, sizeof(ull) * 15);
        } else {
            printf("[!] Unknown command: '%c', ignoring...\n", argv[1][i]);
        }
    }

    handle_config(cfg_path, stats_cfg, true);

    return 0;
    
}