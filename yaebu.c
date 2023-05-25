#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <openssl/md5.h>

#define DB_FILENAME "integrity.db"
#define MAX_PATH_LEN 1024
#define MD5_DIGEST_LENGTH 16
#define MAX_NUM_FILES 100

typedef struct {
    int id;
    char* name;
    char* type;
    int parent_id;
    char* md5;
} IntegrityRecord;

typedef struct {
    char filename[MAX_PATH_LEN];
    char* md5sum;
} FileInfo;

IntegrityRecord record[MAX_NUM_FILES];
int num_records = 0;

char* md5_hash(char* filename) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    char* result = (char*)malloc(MD5_DIGEST_LENGTH*2+1);

    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Error: cannot open file %s\n", filename);
        return NULL;
    }

    MD5_CTX ctx;
    MD5_Init(&ctx);
    unsigned char buffer[1024];
    int bytes = 0;
    while ((bytes = fread(buffer, 1, 1024, fp)) != 0)
        MD5_Update(&ctx, buffer, bytes);
    MD5_Final(digest, &ctx);

    fclose(fp);

    for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
        sprintf(result+i*2, "%02x", digest[i]);
    return result;
}

void write_record(FILE* fp, IntegrityRecord* record) {
    fwrite(&record->id, sizeof(int), 1, fp);
    int len = strlen(record->name)+1;
    fwrite(&len, sizeof(int), 1, fp);
    fwrite(record->name, sizeof(char), len, fp);
    len = strlen(record->type)+1;
    fwrite(&len, sizeof(int), 1, fp);
    fwrite(record->type, sizeof(char), len, fp);
    fwrite(&record->parent_id, sizeof(int), 1, fp);
    if (strcmp(record->type, "file") == 0) {
        len = strlen(record->md5)+1;
        fwrite(&len, sizeof(int), 1, fp);
        fwrite(record->md5, sizeof(char), len, fp);
    }
}

IntegrityRecord* read_record(FILE* fp) {
    IntegrityRecord* record = (IntegrityRecord*)malloc(sizeof(IntegrityRecord));
    fread(&record->id, sizeof(int), 1, fp);
    int len;
    fread(&len, sizeof(int), 1, fp);
    record->name = (char*)malloc(len*sizeof(char));
    fread(record->name, sizeof(char), len, fp);
    fread(&len, sizeof(int), 1, fp);
    record->type = (char*)malloc(len*sizeof(char));
    fread(record->type, sizeof(char), len, fp);
    fread(&record->parent_id, sizeof(int), 1, fp);
    if (strcmp(record->type, "file") == 0) {
        fread(&len, sizeof(int), 1, fp);
        record->md5 = (char*)malloc(len*sizeof(char));
        fread(record->md5, sizeof(char), len, fp);
    }
    return record;
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

    FileInfo file;
    // Get current directory
    char current_dir[MAX_PATH_LEN];
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
        fprintf(stderr, "Error getting current directory\n");
        exit(EXIT_FAILURE);
    }

    // Change current directory to dir_path
    if (chdir(dir_path) != 0) {
        fprintf(stderr, "Error changing directory: %s\n", dir_path);
        exit(EXIT_FAILURE);
    }

    // Write information about root directory
    IntegrityRecord root = {0, dir_path, "directory", -1, NULL};
    write_record(db_file, &root);
    num_records++;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        if (is_directory(path)) {
            IntegrityRecord record = {num_records, entry->d_name, "directory", 0, NULL};
            write_record(db_file, &record);
            num_records++;
            save_subdirectory_info(path, db_file);
        }
        else {
            file.md5sum = md5_hash(path);
            IntegrityRecord record = {num_records, entry->d_name, "file", 0, file.md5sum};
            write_record(db_file, &record);
            num_records++;
        }
    }

    fclose(db_file);
    closedir(dir);
}