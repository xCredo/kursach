#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <openssl/md5.h>
/*#include "libintegrctrl.h"*/
#include "src/LibIntegr/libcrypto.h"

//#define _DEFAULT_SOURCE
#define DB_FILENAME "integrity.db"
#define MAX_PATH_LEN 1024
#define MD5_DIGEST_LENGTH 16

typedef struct {
    char filename[MAX_PATH_LEN];
    unsigned char md5sum[MD5_DIGEST_LENGTH];
} FileInfo;

void calculate_md5sum(char *filename, unsigned char *md5sum) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);

    unsigned char buf[4096];
    int bytes_read;
    while ((bytes_read = fread(buf, 1, sizeof(buf), file)) != 0) {
        MD5_Update(&md5_ctx, buf, bytes_read);
    }

    MD5_Final(md5sum, &md5_ctx);

    fclose(file);
}

int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}


void save_integrity_info(char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        fprintf(stderr, "Error opening directory: %s\n", dir_path);
        exit(EXIT_FAILURE);
    }

    FILE *db_file = fopen(DB_FILENAME, "wb");
    if (db_file == NULL) {
        fprintf(stderr, "Error creating database file\n");
        exit(EXIT_FAILURE);
    }

    FileInfo file_info;
    char path[MAX_PATH_LEN];

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
        {
            continue; // skip hidden files
        }
        snprintf(path, MAX_PATH_LEN, "%s/%s", dir_path, entry->d_name);

        if (is_directory(path)) 
        {
            continue; // skip directories
        }

        // process regular files
        // ...

        snprintf(path, MAX_PATH_LEN, "%s/%s", dir_path, entry->d_name);

        struct stat file_stat;
        if (stat(path, &file_stat) != 0)
        {
            fprintf(stderr, "Error getting file stat: %s\n", path);
            exit(EXIT_FAILURE);
        }

        if (!S_ISREG(file_stat.st_mode)) {
            // skip non-regular files
            continue;
        }

        calculate_md5sum(path, file_info.md5sum);
        strncpy(file_info.filename, entry->d_name, MAX_PATH_LEN);

        fwrite(&file_info, sizeof(FileInfo), 1, db_file);
    }

    closedir(dir);
    fclose(db_file);

    printf("Integrity info saved to %s\n", DB_FILENAME);
}

void check_integrity(char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        fprintf(stderr, "Error opening directory: %s\n", dir_path);
        exit(EXIT_FAILURE);
    }

    FILE *db_file = fopen(DB_FILENAME, "rb");
    if (db_file == NULL) {
        fprintf(stderr, "Error opening database file: %s\n", DB_FILENAME);
        exit(EXIT_FAILURE);
    }

    FileInfo saved_file_info, current_file_info;
    char path[MAX_PATH_LEN];
    int integrity_error_count = 0;

    while (fread(&saved_file_info, sizeof(FileInfo), 1, db_file) != 0) {
        snprintf(path, MAX_PATH_LEN+1, "%s/%s", dir_path, saved_file_info.filename);

        struct stat file_stat;
        if (stat(path, &file_stat) != 0) {
            fprintf(stderr, "Error getting file stat: %s\n", path);
            exit(EXIT_FAILURE);
        }

        if (!S_ISREG(file_stat.st_mode)) {
            // skip non-regular files
            continue;
        }

        calculate_md5sum(path, current_file_info.md5sum);

        if (memcmp(saved_file_info.md5sum, current_file_info.md5sum, MD5_DIGEST_LENGTH) != 0) {
            printf("Integrity error detected for file: %s\n", saved_file_info.filename);
            integrity_error_count++;
        }
    }

    if (integrity_error_count == 0) {
        printf("All files are intact\n");
    } else {
        printf("%d files have integrity errors.\n", integrity_error_count);
    }

    closedir(dir);
    fclose(db_file);
}

int main(int argc, char **argv) {
    if (argc < 3)
    {
        fprintf(stderr, "Usage: integrctrl [-s|-c] <directory_path>\n");
        exit(EXIT_FAILURE);
    }
    if (strcmp(argv[1], "-s") == 0)
    {
        save_integrity_info(argv[2]);
    }
    else if (strcmp(argv[1], "-c") == 0)
    {
        check_integrity(argv[2]);
    } 
    else
    {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}