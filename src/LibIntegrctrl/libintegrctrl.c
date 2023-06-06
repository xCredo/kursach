#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libintegrctrl.h"

char* md5_hash(char* filename) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    char* result = (char*)malloc(MD5_DIGEST_LENGTH*2+1); // calculate the MD5 hash and store it in a dynamically allocated buffer

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
    result[MD5_DIGEST_LENGTH*2]=0;
    return result; // free the dynamically allocated memory
}

void write_integrity_record(FILE* fp, IntegrityRecord* record) {
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

IntegrityRecord* read_integrity_record(FILE* fp) {
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


int check_dir_integrity(FILE* db_file, int parent_id, char* dirname, FileInfo* files, int* num_files) {
    int is_integrity_ok = 1;
    
    char current_dir[MAX_PATH_LEN];
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
        fprintf(stderr, "Error getting current directory\n");
        exit(EXIT_FAILURE);
    }
	
	char *local_dir = strrchr(dirname, '/');
	if(local_dir == NULL) {
		local_dir = dirname;
	} else {
		local_dir++;
	}
    // Изменяем текущую директорию на dir_path
    if (chdir(local_dir) != 0) {
        fprintf(stderr, "Error changing directory: %s\n", dirname);
        exit(EXIT_FAILURE);
    }
	
    // Ищем все записи в базе данных с parent_id
    //fseek(db_file, sizeof(IntegrityRecord), SEEK_SET);
    while (!feof(db_file)) {
        IntegrityRecord* record = read_integrity_record(db_file);
        if (record->parent_id == parent_id && strcmp(record->name, ".") != 0 && strcmp(record->name, "..") != 0) {
            if (strcmp(record->type, "directory") == 0) {
                // Рекурсивно проверяем целостность директории
                
                char path[MAX_PATH_LEN];
                snprintf(path, sizeof(path), "%s/%s", dirname, record->name);
                long int ppos = ftell(db_file);
                is_integrity_ok &= check_dir_integrity(db_file, record->id, path, files, num_files);
                fseek(db_file, ppos, SEEK_SET);
            } else if (strcmp(record->type, "file") == 0) {
                // Ищем файл с таким же именем и сравниваем хеш-функции MD5
                int found = 0;
                printf("CHECKING FILE: %s\n", record->name);
                for (int i = 0; i < *num_files; i++) {
                	char *filename=strrchr(files[i].filename, '/')+1;
                	int dirFilelen = filename-files[i].filename-1;
                	char filedir[MAX_PATH_LEN];
                	strncpy(filedir, files[i].filename, dirFilelen);
                	filedir[dirFilelen]=0;
                    if (strcmp(filedir, dirname)==0 && strcmp(filename, record->name) == 0) {
                        found = 1;
                        if (strcmp(files[i].md5sum, record->md5) != 0) {
                            printf("Error: file %s has been modified\n", files[i].filename);
                            is_integrity_ok = 0;
                        }
                    }
                }
                if (!found) {
                    printf("Error: file %s is missing\n", record->name);
                    is_integrity_ok = 0;
                }
            }
        }
        free(record->name);
        free(record->type);
        if (strcmp(record->type, "file") == 0)
            free(record->md5);
        free(record);
    }
	chdir(current_dir);
    return is_integrity_ok;
}

int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

void save_integrity_info(FILE *db_file, char *dir_path, int rootid) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        fprintf(stderr, "Error opening directory: %s\n", dir_path);
        exit(EXIT_FAILURE);
    }

    

    FileInfo file;
    // Получаем текущую директорию
    char current_dir[MAX_PATH_LEN];
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
        fprintf(stderr, "Error getting current directory\n");
        exit(EXIT_FAILURE);
    }

    // Изменяем текущую директорию на dir_path
    if (chdir(dir_path) != 0) {
        fprintf(stderr, "Error changing directory: %s\n", dir_path);
        exit(EXIT_FAILURE);
    }

    // Записываем информацию о корневой директории
    
    if(rootid == -1) {
    	IntegrityRecord root;
	    root.id = get_new_id();
	    root.name = strdup(".");
	    root.type = strdup("directory");
	    root.parent_id = 0;
	    root.md5 = NULL;
	    write_integrity_record(db_file, &root);
	    
	    rootid = root.id;
	}
    

    // Обрабатываем каждый файл или директорию в текущей директории
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "%s/%s", dir_path, ent->d_name);
		printf("%s\n", path);
        if (is_directory(ent->d_name)) {
            // Рекурсивно обрабатываем поддиректорию
            

            // Создаем запись для директории
            IntegrityRecord dir_record;
            dir_record.id = get_new_id();
            dir_record.name = strdup(ent->d_name);
            dir_record.type = strdup("directory");
            dir_record.parent_id = rootid;
            dir_record.md5 = NULL;
            write_integrity_record(db_file, &dir_record);
            
            save_integrity_info(db_file, ent->d_name, dir_record.id);
        } 
        
        else 
        {
            // Вычисляем хеш-функцию MD5 для файла
            char* md5sum = md5_hash(ent->d_name);
            // Создаем запись для файла
            IntegrityRecord file_record;
            file_record.id = get_new_id();
            file_record.name = strdup(ent->d_name);
            file_record.type = strdup("file");
            file_record.parent_id = rootid;
            file_record.md5 = md5sum;
            write_integrity_record(db_file, &file_record);

            // // Запоминаем имя файла и его хеш-функцию для дальнейшей проверки целостности
            // FileInfo file_info;
            // snprintf(file_info.filename, sizeof(file_info.filename), "%s/%s", dir_path, ent->d_name);
            // file_info.md5sum = md5sum;
            // Добавляем информацию о файле в массив
            if (num_files < MAX_NUM_FILES)
            {
                snprintf(file_info[num_files].filename, sizeof(file_info[num_files].filename), "%s/%s", dir_path, ent->d_name);
                file_info[num_files].md5sum = md5sum;
                num_files++;
            }
            else
            {
                printf("Error: too many files to store in memory\n");
            }
            free(md5sum);
        }
    }


    if (chdir(current_dir) != 0) {
        fprintf(stderr, "Error changing directory: %s\n", current_dir);
        exit(EXIT_FAILURE);
    }

    printf("Integrity information saved to file: %s\n", DB_FILENAME);
}


void get_files(const char *dir_path) {
	
	char *local_dir = strrchr(dir_path, '/');
	if(local_dir==NULL) {
		local_dir = dir_path-1;
	}
    DIR *dir = opendir(local_dir+1);
    if (dir == NULL) {
        fprintf(stderr, "Error opening directory: %s\n", dir_path);
        exit(EXIT_FAILURE);
    }
    FileInfo file;
    // Получаем текущую директорию
    char current_dir[MAX_PATH_LEN];
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
        fprintf(stderr, "Error getting current directory\n");
        exit(EXIT_FAILURE);
    }

    // Изменяем текущую директорию на dir_path
    if (chdir(local_dir+1) != 0) {
        fprintf(stderr, "Error changing directory: %s\n", dir_path);
        exit(EXIT_FAILURE);
    }

    // Записываем информацию о корневой директории

    // Обрабатываем каждый файл или директорию в текущей директории
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "%s/%s", dir_path, ent->d_name);
        if (is_directory(ent->d_name)) {
            // Рекурсивно обрабатываем поддиректорию
            get_files(path);
        } 
        
        else 
        {
            // Вычисляем хеш-функцию MD5 для файла
            char* md5sum = md5_hash(ent->d_name);
            // Создаем запись для файла
            // // Запоминаем имя файла и его хеш-функцию для дальнейшей проверки целостности
            // FileInfo file_info;
            // snprintf(file_info.filename, sizeof(file_info.filename), "%s/%s", dir_path, ent->d_name);
            // file_info.md5sum = md5sum;
            // Добавляем информацию о файле в массив
            if (num_files < MAX_NUM_FILES)
            {
                snprintf(file_info[num_files].filename, sizeof(file_info[num_files].filename), "%s/%s", dir_path, ent->d_name);
                file_info[num_files].md5sum = md5sum;
                num_files++;
            }
            else
            {
                printf("Error: too many files to store in memory\n");
            }
        }
    }

    if (chdir(current_dir) != 0) {
        fprintf(stderr, "Error changing directory: %s\n", current_dir);
        exit(EXIT_FAILURE);
    }

}

int check_integrity(const char* dirpath) {
    FILE *db_file = fopen(DB_FILENAME, "rb");
    if (db_file == NULL) {
        fprintf(stderr, "Error opening database file\n");
        exit(EXIT_FAILURE);
    }
    
    
    
    if (chdir(dirpath) != 0) {
        fprintf(stderr, "Error changing directory: %s\n", dirpath);
        exit(EXIT_FAILURE);
    }
    
    get_files(".");

    // Считываем первую запись, это должна быть запись корневой директории
    IntegrityRecord *root_record = read_integrity_record(db_file);
    if (strcmp(root_record->name, ".") != 0 || strcmp(root_record->type, "directory") != 0) {
        fprintf(stderr, "Error: invalid database file format\n");
        fclose(db_file);
        exit(EXIT_FAILURE);
    }

    // Создаем массив с информацией о файлах
    //FileInfo* files = (FileInfo*)malloc(100 * sizeof(FileInfo));
    //int num_files = 0;

    // Рекурсивно проверяем целостность директории
    
    printf("ROOTID: %d\n", root_record->id);
    int is_integrity_ok = 1;
    is_integrity_ok &= check_dir_integrity(db_file, root_record->id, ".", file_info, &num_files);

    fclose(db_file);

    if (!is_integrity_ok)
        printf("Integrity check failed.\n");
    else
        printf("Integrity check passed.\n");


    return is_integrity_ok;
}

void print_results(FileInfo* files, int num_files) {
    for (int i = 0; i < num_files; i++) {
        if (files[i].md5sum != NULL) {
            printf("%s\n", files[i].filename);
        }
    }
}