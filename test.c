Для написания тестов мы можем использовать библиотеку CTest.h. Например, для тестирования функции `md5_hash` можно написать следующий тест:

```c
#include <ctest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "integrity.h"

CTEST(md5_test, md5_hash) {
    char* filename = "test_file.txt";
    FILE* fp = fopen(filename, "w");
    fprintf(fp, "This is a test file for md5 hashing");
    fclose(fp);

    char* expected_hash = "6d99af58f59b4df8acf82a4bb2f212ec";
    char* actual_hash = md5_hash(filename);

    ASSERT_STR(expected_hash, actual_hash);

    remove(filename);
    free(actual_hash);
}
```

Аналогично, для тестирования функций `write_integrity_record` и `read_integrity_record` можно написать следующий тест:

```c
CTEST(integrity_test, write_and_read_integrity_record) {
    IntegrityRecord record;
    record.id = get_new_id();
    record.name = "testfile.txt";
    record.type = "file";
    record.parent_id = 0;
    record.md5 = "6d99af58f59b4df8acf82a4bb2f212ec";

    FILE* fp = fopen(DB_FILENAME, "wb");
    write_integrity_record(fp, &record);
    fclose(fp);

    fp = fopen(DB_FILENAME, "rb");
    IntegrityRecord* read_record = read_integrity_record(fp);

    ASSERT_EQUAL(record.id, read_record->id);
    ASSERT_STR(record.name, read_record->name);
    ASSERT_STR(record.type, read_record->type);
    ASSERT_EQUAL(record.parent_id, read_record->parent_id);
    ASSERT_STR(record.md5, read_record->md5);

    fclose(fp);
    free(read_record->name);
    free(read_record->type);
    free(read_record->md5);
    free(read_record);
}
```

Наконец, для тестирования функции `check_dir_integrity` можно написать следующий тест:

```c
CTEST(integrity_test, check_dir_integrity) {
    // Создаем временную директорию и файлы в ней
    char* dirname = "testdir";
    mkdir(dirname, 0777);
    FILE* fp = fopen(DB_FILENAME, "wb");
    fprintf(fp, "%d\n", 0);
    IntegrityRecord record;
    record.id = get_new_id();
    record.name = dirname;
    record.type = "directory";
    record.parent_id = 0;
    write_integrity_record(fp, &record);
    num_files = 0;
    FileInfo file1;
    strcpy(file1.filename, "testdir/testfile1.txt");
    file1.md5sum = md5_hash("testdir/testfile1.txt");
    file_info[num_files++] = file1;
    IntegrityRecord file1_record;
    file1_record.id = get_new_id();
    file1_record.name = "testfile1.txt";
    file1_record.type = "file";
    file1_record.parent_id = record.id;
    file1_record.md5 = file1.md5sum;
    write_integrity_record(fp, &file1_record);
    FileInfo file2;
    strcpy(file2.filename, "testdir/testfile2.txt");
    file2.md5sum = md5_hash("testdir/testfile2.txt");
    file_info[num_files++] = file2;
    IntegrityRecord file2_record;
    file2_record.id = get_new_id();
    file2_record.name = "testfile2.txt";
    file2_record.type = "file";
    file2_record.parent_id = record.id;
    file2_record.md5 = file2.md5sum;
    write_integrity_record(fp, &file2_record);
    fclose(fp);

    // Изменяем содержимое одного из файлов
    fp = fopen("testdir/testfile2.txt", "w");
    fprintf(fp, "This is a modified test file");
    fclose(fp);

    // Проверяем целостность директории
    fp = fopen(DB_FILENAME, "rb");
    int is_integrity_ok = check_dir_integrity(fp, 0, dirname, file_info, &num_files);
    fclose(fp);

    ASSERT_FALSE(is_integrity_ok);

    // Очищаем ресурсы
    remove("testdir/testfile1.txt");
    remove("testdir/testfile2.txt");
    rmdir(dirname);
    free(file2.md5sum);
}
