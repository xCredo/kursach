#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <openssl/md5.h>
#include <unistd.h>

#define DB_FILENAME "integrity.db"
#define MAX_PATH_LEN 1024
#define MD5_DIGEST_LENGTH 16
#define MAX_NUM_FILES 100

typedef struct {
    char filename[MAX_PATH_LEN];
    char* md5sum;
} FileInfo;

FileInfo file_info[MAX_NUM_FILES];
int num_files = 0;

typedef struct {
    int id;             // Уникальный идентификатор записи
    char* name;         // Имя директории или файла
    char* type;         // Тип: директория или файл
    int parent_id;      // Уникальный идентификатор родительской директории
    char* md5;          // Хеш-функция MD5, вычисленная для содержимого файла
} IntegrityRecord;

int get_new_id();
char* md5_hash(char* filename);
void write_integrity_record(FILE* fp, IntegrityRecord* record);
IntegrityRecord* read_integrity_record(FILE* fp);
int check_dir_integrity(FILE* db_file, int parent_id, char* dirname, FileInfo* files, int* num_files);
int is_directory(const char *path);
void save_integrity_info(FILE *db_file, char *dir_path, int rootid);
void get_files(const char *dir_path);
int check_integrity(const char* dirpath);
void print_results(FileInfo* files, int num_files);